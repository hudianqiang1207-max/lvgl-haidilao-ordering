/**
 * @file haidi_server.c
 * @brief 海底捞点餐平板 - PC 服务器端（C/S 框架）
 *
 * 开发板(客户端) -> PC(服务器) 的 TCP 服务程序。
 * 功能：菜单下发、库存管理(售罄)、订单接收、模拟支付、出货控制。
 * 管理端：默认在客户端端口+1提供 HTML 管理台，例如 8889；支持菜单增删改查、库存维护、订单查看。
 * 协议：客户端使用行文本(以 \n 结尾)，管理端使用 HTTP/JSON。
 *
 * 编译（Linux/WSL）：
 *   gcc -Wall -O2 -o haidi_server haidi_server.c -lpthread
 * 编译（Windows MinGW）：
 *   gcc -Wall -O2 -o haidi_server.exe haidi_server.c -lws2_32
 * 运行：./haidi_server [端口]   （默认 8888，监听 0.0.0.0）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#ifndef _WIN32
#include <pthread.h>
#endif

/* ---------- 跨平台 socket 兼容层 ---------- */
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define CLOSE_SOCK(s) closesocket(s)
    #define SOCK_ERR INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define CLOSE_SOCK(s) close(s)
    #define SOCK_ERR (-1)
    #define SOCKET int
#endif

/* ---------- 配置 ---------- */
#define DEFAULT_PORT    8888
#define ADMIN_PORT_OFFSET 1       /* 网页管理端口：客户端端口 + 1 */
#define MAX_CLIENTS     16
#define RECV_BUF        4096
#define MENU_MAX        80
#define ORDER_ITEMS_MAX 12

/* ---------- 菜单菜品 ---------- */
typedef struct {
    int id;
    char name[32];
    char desc[48];
    int price;
    int category;   /* 0=招牌 1=肉类 2=海鲜 3=蔬菜 4=主食 5=饮品 */
    int stock;      /* 库存，0 表示售罄 */
} menu_item_t;

static menu_item_t g_menu[MENU_MAX];
static int g_menu_count = 0;

/* ---------- 订单 ---------- */
typedef struct {
    int order_id;
    int items[ORDER_ITEMS_MAX][2];  /* [i][0]=菜单id [i][1]=数量 */
    int item_count;
    int total;      /* 总价(元) */
    int state;      /* 0=已下单 1=已支付 2=已出货 */
} order_t;

#define ORDER_MAX 128
static order_t g_orders[ORDER_MAX];
static int g_order_count = 0;
static int g_next_order_id = 10001;
/* 菜单修改序号：管理台每次增删改库存后递增，客户端据此可靠同步。 */
static unsigned long g_menu_revision = 1;
#ifndef _WIN32
static pthread_mutex_t g_data_mutex = PTHREAD_MUTEX_INITIALIZER;
#define DATA_LOCK() pthread_mutex_lock(&g_data_mutex)
#define DATA_UNLOCK() pthread_mutex_unlock(&g_data_mutex)
#else
#define DATA_LOCK() ((void)0)
#define DATA_UNLOCK() ((void)0)
#endif

/* ---------- 客户端会话 ---------- */
typedef struct {
    SOCKET fd;
    char buf[RECV_BUF];
    int len;
    int active;
} client_t;

static client_t g_clients[MAX_CLIENTS];

/* ================================================================
 *  菜单数据（与客户端 ui/menu_data.c 保持一致，附库存）
 * ================================================================ */
static void menu_init(void)
{
    g_menu_count = 0;
    /* id, name, desc, price, category, stock */
    #define ADD(ID, NAME, DESC, PRICE, CAT, STOCK) do { \
        menu_item_t *m = &g_menu[g_menu_count++]; \
        m->id = ID; strcpy(m->name, NAME); strcpy(m->desc, DESC); \
        m->price = PRICE; m->category = CAT; m->stock = STOCK; \
    } while (0)

    ADD(368,  "新一代番茄火锅", "一定要喝汤的番茄火锅", 68, 0, 20);
    ADD(3307, "经典麻辣火锅",   "慢火熬煮4小时的经典麻辣锅底", 88, 0, 20);
    ADD(3493, "清油麻辣火锅",   "植物油炒制，香辣浓郁不油腻", 78, 0, 20);
    ADD(3494, "三鲜火锅",       "色泽纯正，口感鲜香浓郁", 58, 0, 20);
    ADD(3495, "岭南黄猪肚鸡火锅", "汤头乳白、鲜香浓郁", 98, 0, 15);
    ADD(7634, "浓浓浓菌汤锅",   "研发300天，菌汤超级鲜", 62, 0, 20);
    ADD(7676, "真香锅",         "汤底鲜香，麻辣适中，清爽不油腻", 82, 0, 15);
    ADD(428,  "捞派捞面",       "明星产品，现场捞面表演", 12, 1, 20);
    ADD(3510, "捞派脆脆毛肚",   "毛肚即牛胃，涮8秒最脆", 58, 1, 10);
    ADD(8267, "捞派鸭肠",       "每一根都口感爽脆有韧性", 42, 1, 10);
    ADD(8268, "捞派豆花",       "传统手工工艺压制，嫩滑", 18, 1, 15);
    ADD(8269, "捞派麻辣滑牛肉", "精选牛小米龙部位，嫩滑", 62, 1, 10);
    ADD(8270, "招牌大颗粒虾滑", "虾肉含量高，鲜甜Q弹", 52, 1, 12);
    ADD(8271, "捞派嫩羊肉",     "优质羊后腿肉，口感嫩滑", 55, 1, 10);
    ADD(8272, "捞派黄喉",       "精选猪黄喉，去筋膜去腥", 46, 1, 10);
    ADD(8273, "捞派巴沙鱼片",   "越南巴沙鱼，少刺鲜嫩", 38, 1, 12);
    ADD(8679, "8秒海带苗",      "8秒涮烫，一口脆爽", 16, 1, 15);
    ADD(8306, "捞派肥牛",       "进口谷饲牛肉，肥瘦相间", 58, 2, 12);
    ADD(8307, "精品腹肉肥牛",   "肥瘦相间，口感醇厚", 62, 2, 10);
    ADD(8308, "精选牛板腱",     "雪花丰富，肉香浓郁", 66, 2, 10);
    ADD(8309, "新西兰羔羊羊排卷", "10个月小羔羊，肉中贵族", 68, 2, 8);
    ADD(8310, "内蒙特色沙葱羊肉卷", "草原羔羊+沙葱清香", 60, 2, 10);
    ADD(8311, "精品肥羊1号",    "前肩部位肉，肥瘦搭配", 56, 2, 10);
    ADD(8312, "层层雪浪猪五花", "肥瘦相间、香而不腻", 45, 2, 12);
    ADD(8313, "火锅牛排",       "牛板腱部位，鲜嫩多汁", 72, 2, 8);
    ADD(8314, "我的小郡肝",     "鸡胗切花，爽脆咸鲜麻辣", 32, 2, 15);
    ADD(8315, "鲜切羊肉",       "冷鲜腿肉，瘦而不硬", 52, 2, 10);
    ADD(8316, "脱骨鸭掌",       "剔硬骨保留完整性，劲道", 35, 2, 12);
    ADD(8317, "猪脑花",         "精选优质猪脑，细腻嫩滑", 28, 2, 8);
    ADD(8318, "鸭舌",           "优质鸭舌，肉嫩鲜美", 38, 2, 10);
    ADD(8319, "脆皮肠",         "优质鸡肉，爽脆烟熏味", 22, 2, 15);
    ADD(8320, "午餐肉",         "精选猪后腿肉，肉香浓郁", 25, 2, 15);
    ADD(8499, "精选安格斯牛板腱", "连筋带肉，有嚼劲", 78, 2, 8);
    ADD(8502, "臻选安格斯牛板腱", "只用安格斯牛，肉嫩多汁", 88, 2, 6);
    ADD(8503, "经典连筋带肉牛板腱", "板腱取牛肩胛部位", 75, 2, 8);
    ADD(8504, "川式香辣滑牛肉", "裹满辣椒面，又香又辣", 55, 2, 10);
    ADD(8498, "莓莓抹茶牛乳",   "抹茶+草莓果酱，惊喜口感", 22, 3, 12);
    ADD(8501, "雪花肩胛小排",   "牛肋骨3-5根之间，肉香足", 68, 3, 8);
    ADD(8674, "内蒙罕山白绒山羊", "赤峰羊肉，地理标志产品", 66, 3, 8);
    ADD(8675, "虫草花乌鸡丸",   "乌鸡虫草花，营养高蛋白", 42, 3, 10);
    ADD(8676, "雪人豆乳蛋糕",   "有颜有料，双重口感", 18, 3, 15);
    ADD(8677, "虾滑鱼籽三明治", "三层口感三层鲜", 48, 3, 10);
    ADD(8678, "黑松露鱼籽牛肉饭", "鱼籽松露香，一口香迷糊", 58, 3, 8);
    ADD(130,  "现炸酥肉",       "酥脆可口，椒香口味", 28, 4, 15);
    ADD(3604, "茴香小油条",     "鲜茴香，外酥里软", 12, 4, 20);
    ADD(3608, "五常米饭",       "米香浓郁，软糯可口", 3, 4, 50);
    ADD(8321, "一口爆浆奶酪鱼条", "外酥里嫩，奶酪味十足", 32, 4, 12);
    ADD(8322, "酥酥嫩嫩炸牛奶", "口感酥脆，老少皆宜", 18, 4, 15);
    ADD(8525, "你龙我龙慕斯(白桃)", "清甜可口，桃香四溢", 22, 4, 12);
    ADD(8526, "喜乐龙龙慕斯(蜜瓜)", "蜜瓜清新，口感细腻", 22, 4, 12);
    ADD(456,  "特色蘸料",       "锅底百搭", 6, 5, 30);
    ADD(3325, "鲜香味碟",       "提鲜必备", 6, 5, 30);
    ADD(3594, "香辣味碟",       "川渝搭档", 6, 5, 30);
    ADD(3595, "海鲜味碟",       "海鲜专属", 6, 5, 30);
    ADD(3596, "丸滑味碟",       "丸滑必备", 6, 5, 30);
    ADD(3597, "酸辣味碟",       "酸辣开胃", 6, 5, 30);
    ADD(3598, "蒜泥香油碟",     "川式口味", 6, 5, 30);
    ADD(3599, "芝麻调味酱",     "老北京口味", 6, 5, 30);
    ADD(40,   "海底捞大麦拉格啤酒", "天禄奖一星，甄选大麦芽", 18, 6, 12);
    ADD(3618, "海底捞德式小麦啤酒", "CCBA银奖，果香馥郁", 18, 6, 12);
    ADD(3762, "海底捞淡爽拉格啤酒", "CCBA铜奖，酒体金黄", 16, 6, 12);
    ADD(21,   "酸梅汤",         "自制酸梅·冰爽解腻", 8, 6, 6);
    #undef ADD

    srand((unsigned)time(NULL));
    printf("[server] menu loaded: %d items\n", g_menu_count);
}

/* ================================================================
 *  工具函数
 * ================================================================ */
static menu_item_t *menu_find(int id)
{
    for (int i = 0; i < g_menu_count; i++)
        if (g_menu[i].id == id) return &g_menu[i];
    return NULL;
}

static order_t *order_find(int id)
{
    for (int i = 0; i < g_order_count; i++)
        if (g_orders[i].order_id == id) return &g_orders[i];
    return NULL;
}

static int send_all(SOCKET fd, const char *buf, int len)
{
    int sent = 0;
    while (sent < len) {
        int n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

static void send_line(SOCKET fd, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
    va_end(ap);
    int n = (int)strlen(buf);
    buf[n++] = '\n';
    buf[n] = 0;
    send_all(fd, buf, n);
}

/* ================================================================
 *  协议处理
 * ================================================================ */
static unsigned long menu_version(void)
{
    unsigned long v = g_menu_revision * 16777619u;
    for (int i = 0; i < g_menu_count; i++) {
        menu_item_t *m = &g_menu[i];
        v = (v ^ (unsigned long)m->id) * 16777619u;
        v = (v ^ (unsigned long)m->price) * 16777619u;
        v = (v ^ (unsigned long)m->category) * 16777619u;
        v = (v ^ (unsigned long)m->stock) * 16777619u;
        for (const unsigned char *s = (const unsigned char *)m->name; *s; s++) v = (v ^ *s) * 16777619u;
        for (const unsigned char *s = (const unsigned char *)m->desc; *s; s++) v = (v ^ *s) * 16777619u;
    }
    return v;
}

static void cmd_menu_version(SOCKET fd)
{
    DATA_LOCK();
    unsigned long v = menu_version();
    DATA_UNLOCK();
    /* 协议版本固定为32位十进制，兼容ARM的32位unsigned long。 */
    send_line(fd, "MENU_VERSION %u", (unsigned int)v);
    fflush(stdout);
}

static void cmd_get_menu(SOCKET fd)
{
    DATA_LOCK();
    char tmp[256];
    int n = snprintf(tmp, sizeof(tmp), "MENU %d", g_menu_count);
    tmp[n++] = '\n';
    send_all(fd, tmp, n);

    for (int i = 0; i < g_menu_count; i++) {
        menu_item_t *m = &g_menu[i];
        send_line(fd, "ITEM %d|%s|%s|%d|%d|%d",
                  m->id, m->name, m->desc, m->price, m->category, m->stock);
    }
    DATA_UNLOCK();
}

static void cmd_order(SOCKET fd, const char *body)
{
    /* ORDER id:qty,id:qty,... */
    int ids[ORDER_ITEMS_MAX], qtys[ORDER_ITEMS_MAX], cnt = 0;
    char tmp[256];
    strncpy(tmp, body, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *tok = strtok(tmp, ", ");
    while (tok && cnt < ORDER_ITEMS_MAX) {
        int id = 0, q = 0;
        if (sscanf(tok, "%d:%d", &id, &q) == 2 && q > 0) {
            ids[cnt] = id;
            qtys[cnt] = q;
            cnt++;
        }
        tok = strtok(NULL, ", ");
    }
    if (cnt == 0) { send_line(fd, "ORDER_FAIL empty"); return; }

    /* 校验库存 */
    for (int i = 0; i < cnt; i++) {
        menu_item_t *m = menu_find(ids[i]);
        if (!m) { send_line(fd, "ORDER_FAIL no_item_%d", ids[i]); return; }
        if (m->stock < qtys[i]) { send_line(fd, "ORDER_FAIL soldout_%d", ids[i]); return; }
    }

    /* 扣库存、建订单 */
    order_t *o = &g_orders[g_order_count++];
    if (g_order_count >= ORDER_MAX) g_order_count = ORDER_MAX - 1;
    o->order_id = g_next_order_id++;
    o->item_count = cnt;
    o->total = 0;
    o->state = 0;
    for (int i = 0; i < cnt; i++) {
        menu_item_t *m = menu_find(ids[i]);
        m->stock -= qtys[i];
        o->items[i][0] = ids[i];
        o->items[i][1] = qtys[i];
        o->total += m->price * qtys[i];
    }
    send_line(fd, "ORDER_OK %d %d", o->order_id, o->total);

    /* 打印订单明细（订单号 + 每道菜 + 总价） */
    printf("========================================\n");
    printf("[订单] 订单号: %d\n", o->order_id);
    for (int i = 0; i < o->item_count; i++) {
        menu_item_t *m = menu_find(o->items[i][0]);
        if (m) {
            printf("  %-14s x%d = %d元\n", m->name, o->items[i][1], m->price * o->items[i][1]);
        }
    }
    printf("  合计: %d元 (%d 种菜品)\n", o->total, o->item_count);
    printf("========================================\n");
}

static void cmd_pay(SOCKET fd, const char *body)
{
    /* PAY order_id method(1微信 2支付宝 3现金) */
    int id = 0, method = 0;
    if (sscanf(body, "%d %d", &id, &method) != 2) {
        send_line(fd, "PAY_FAIL bad_args");
        return;
    }
    order_t *o = order_find(id);
    if (!o) { send_line(fd, "PAY_FAIL no_order"); return; }
    if (o->state >= 1) { send_line(fd, "PAY_FAIL already_paid"); return; }

    /* 模拟支付：成功概率 95%，少量演示失败场景 */
    if (rand() % 100 < 95) {
        o->state = 1;
        send_line(fd, "PAY_OK %d %d", o->order_id, o->total);
        {
        const char *mname = method == 1 ? "微信支付" : method == 2 ? "支付宝" : method == 3 ? "现金支付" : "未知";
        printf("[订单] 订单 %d 已支付（%s）\n", id, mname);
    }
    } else {
        send_line(fd, "PAY_FAIL network");
    }
}

static void cmd_dispense(SOCKET fd, const char *body)
{
    /* DISPENSE order_id */
    int id = 0;
    if (sscanf(body, "%d", &id) != 1) {
        send_line(fd, "DISPENSE_FAIL bad_args");
        return;
    }
    order_t *o = order_find(id);
    if (!o) { send_line(fd, "DISPENSE_FAIL no_order"); return; }
    if (o->state < 1) { send_line(fd, "DISPENSE_FAIL not_paid"); return; }
    if (o->state >= 2) { send_line(fd, "DISPENSE_FAIL already_done"); return; }

    /* 模拟出货：成功率 96% */
    if (rand() % 100 < 96) {
        o->state = 2;
        send_line(fd, "DISPENSE_OK %d", o->order_id);
        printf("[订单] 订单 %d 已出货\n", id);
    } else {
        send_line(fd, "DISPENSE_FAIL jam");   /* 模拟卡货故障 */
    }
}

static void cmd_stock(SOCKET fd, const char *body)
{
    /* STOCK id new_stock （管理/测试用） */
    int id = 0, st = 0;
    if (sscanf(body, "%d %d", &id, &st) != 2) {
        send_line(fd, "STOCK_FAIL bad_args");
        return;
    }
    menu_item_t *m = menu_find(id);
    if (!m) { send_line(fd, "STOCK_FAIL no_item"); return; }
    m->stock = st;
    g_menu_revision++;
    send_line(fd, "STOCK_OK %d %d", id, m->stock);
    printf("[server] stock[%d] set to %d\n", id, st);
}

static void process_line(SOCKET fd, const char *line)
{
    if (strncmp(line, "MENU_VERSION", 12) == 0) {
        cmd_menu_version(fd);
    } else if (strncmp(line, "GET_MENU", 8) == 0) {
        cmd_get_menu(fd);
    } else if (strncmp(line, "ORDER ", 6) == 0) {
        cmd_order(fd, line + 6);
    } else if (strncmp(line, "PAY ", 4) == 0) {
        cmd_pay(fd, line + 4);
    } else if (strncmp(line, "DISPENSE ", 9) == 0) {
        cmd_dispense(fd, line + 9);
    } else if (strncmp(line, "STOCK ", 6) == 0) {
        cmd_stock(fd, line + 6);
    } else if (strncmp(line, "STATUS", 6) == 0) {
        send_line(fd, "STATUS_OK haidi_server_v1 orders=%d", g_order_count);
    } else if (strncmp(line, "QUIT", 4) == 0) {
        send_line(fd, "BYE");
    } else {
        send_line(fd, "ERR unknown_cmd");
    }
}


/* ================================================================
 *  HTTP 管理端：默认监听客户端端口+1，例如 8889
 *  仅依赖 libc/pthreads，不引入第三方 Web 框架。
 * ================================================================ */
#ifndef _WIN32
static const char admin_html[] =
"<!doctype html><html lang='zh-CN'><meta charset='utf-8'><title>海底捞管理台</title>"
"<style>body{font:14px Arial;margin:24px;background:#f5f5f5}h1{color:#c9151e}.bar{background:#fff;padding:12px;margin:10px 0}input,button{padding:7px;margin:3px}button{cursor:pointer}table{background:#fff;border-collapse:collapse;width:100%}td,th{border:1px solid #ddd;padding:7px;text-align:left}th{background:#c9151e;color:#fff}.danger{color:#c00}</style>"
"<h1>海底捞点餐管理台</h1><div class='bar'><button onclick='load()'>刷新菜单</button><button onclick='orders()'>查看订单</button><label><input id='auto' type='checkbox' checked>订单自动刷新</label><span id='msg'></span></div>"
"<div class='bar'><input id='id' placeholder='ID'><input id='name' placeholder='菜品名称'><input id='desc' placeholder='描述'><input id='price' type='number' placeholder='价格'><input id='cat' type='number' placeholder='分类0-6'><input id='stock' type='number' placeholder='库存'><button onclick='save()'>新增/修改</button></div>"
"<table><thead><tr><th>ID</th><th>名称</th><th>描述</th><th>价格</th><th>分类</th><th>库存</th><th>操作</th></tr></thead><tbody id='rows'></tbody></table>"
"<script>var data=[];var view='menu';function msg(x){document.getElementById('msg').textContent=x}function load(){view='menu';document.querySelector('thead').innerHTML='<tr><th>ID</th><th>名称</th><th>描述</th><th>价格</th><th>分类</th><th>库存</th><th>操作</th></tr>';fetch('/api/menu').then(r=>r.json()).then(a=>{data=a;rows.innerHTML=a.map(function(x){return '<tr><td>'+x.id+'</td><td>'+x.name+'</td><td>'+x.desc+'</td><td>'+x.price+'</td><td>'+x.category+'</td><td>'+x.stock+'</td><td><button onclick=edit('+x.id+')>编辑</button><button class=danger onclick=del('+x.id+')>删除</button></td></tr>'}).join('');msg('已加载 '+a.length+' 条菜单')})}function edit(id){var x=data.find(function(a){return a.id==id});['id','name','desc','price','cat','stock'].forEach(function(k){document.getElementById(k).value=k=='cat'?x.category:x[k]})}function save(){var x={id:+document.getElementById('id').value,name:document.getElementById('name').value,desc:document.getElementById('desc').value,price:+document.getElementById('price').value,category:+document.getElementById('cat').value,stock:+document.getElementById('stock').value};var method=data.some(function(a){return a.id==x.id})?'PUT':'POST';fetch('/api/menu'+(method=='PUT'?'/'+x.id:''),{method:method,headers:{'Content-Type':'application/json'},body:JSON.stringify(x)}).then(r=>r.text()).then(function(t){msg(t);load()})}function del(i){if(confirm('确定删除?'))fetch('/api/menu/'+i,{method:'DELETE'}).then(r=>r.text()).then(function(t){msg(t);load()})}function orders(){view='orders';fetch('/api/orders').then(r=>r.json()).then(function(a){document.querySelector('thead').innerHTML='<tr><th>订单号</th><th>菜品明细</th><th>总金额</th><th>状态</th><th>说明</th></tr>';rows.innerHTML=a.map(function(x){var detail=(x.items||[]).map(function(i){return i.name+' × '+i.quantity+'（'+i.price+'元，小计 '+i.subtotal+'元）'}).join('<br>');return '<tr><td>'+x.order_id+'</td><td>'+detail+'</td><td>'+x.total+' 元</td><td>'+['待支付','已支付','已出货'][x.state]+'</td><td>服务端实时数据</td></tr>'}).join('');msg('订单 '+a.length+' 条')})}setInterval(function(){if(view==='orders' && document.getElementById('auto').checked)orders()},3000);load()</script></html>";
static void admin_send(int fd, const char *status, const char *type, const char *body){char h[256];int n=(int)strlen(body);snprintf(h,sizeof(h),"HTTP/1.1 %s\r\nContent-Type: %s; charset=utf-8\r\nContent-Length: %d\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n",status,type,n);send(fd,h,(int)strlen(h),0);send(fd,body,n,0);}
static void admin_menu_json(char *out,int size){int pos=0;out[pos++]='[';for(int i=0;i<g_menu_count;i++){menu_item_t*m=&g_menu[i];pos+=snprintf(out+pos,size-pos,"%s{\"id\":%d,\"name\":\"%s\",\"desc\":\"%s\",\"price\":%d,\"category\":%d,\"stock\":%d}",i?",":"",m->id,m->name,m->desc,m->price,m->category,m->stock);if(pos>=size-32)break;}snprintf(out+pos,size-pos,"]");}
static void admin_orders_json(char *out,int size){int pos=0;out[pos++]='[';for(int i=0;i<g_order_count;i++){order_t*o=&g_orders[i];pos+=snprintf(out+pos,size-pos,"%s{\"order_id\":%d,\"total\":%d,\"state\":%d,\"items\":[",i?",":"",o->order_id,o->total,o->state);for(int j=0;j<o->item_count;j++){menu_item_t*m=menu_find(o->items[j][0]);if(!m)continue;pos+=snprintf(out+pos,size-pos,"%s{\"id\":%d,\"name\":\"%s\",\"quantity\":%d,\"price\":%d,\"subtotal\":%d}",j?",":"",m->id,m->name,o->items[j][1],m->price,m->price*o->items[j][1]);}pos+=snprintf(out+pos,size-pos,"]}");if(pos>=size-256)break;}snprintf(out+pos,size-pos,"]");}
#define ADMIN_LOCK() DATA_LOCK()
#define ADMIN_UNLOCK() DATA_UNLOCK()
static int json_int(const char *b,const char*k,int def){char pat[64];snprintf(pat,sizeof(pat),"\"%s\":",k);const char*p=strstr(b,pat);return p?atoi(p+strlen(pat)):def;}
static void json_str(const char*b,const char*k,char*out,int n,const char*def){char pat[64];snprintf(pat,sizeof(pat),"\"%s\":\"",k);const char*p=strstr(b,pat);if(!p){snprintf(out,n,"%s",def);return;}p+=strlen(pat);int i=0;while(*p && *p != '"' && i<n-1) out[i++]=*p++;out[i]=0;}
static void admin_request(int fd){
 char req[8192]; int n=(int)recv(fd,req,sizeof(req)-1,0); if(n<=0)return; req[n]=0;
 char method[8]={0},path[128]={0}; sscanf(req,"%7s %127s",method,path);
 if(!strcmp(method,"GET")&&!strcmp(path,"/")){admin_send(fd,"200 OK","text/html",admin_html);return;}
 char body[8192]={0}; char *sep=strstr(req,"\r\n\r\n"); if(sep)snprintf(body,sizeof(body),"%s",sep+4); if(body[0]==0){sep=strstr(req,"\n\n"); if(sep)snprintf(body,sizeof(body),"%s",sep+2);}
 if(!strcmp(method,"GET")&&!strcmp(path,"/api/menu")){char out[12000];ADMIN_LOCK();admin_menu_json(out,sizeof(out));ADMIN_UNLOCK();admin_send(fd,"200 OK","application/json",out);return;}
 if(!strcmp(method,"GET")&&!strcmp(path,"/api/orders")){char out[8192];ADMIN_LOCK();admin_orders_json(out,sizeof(out));ADMIN_UNLOCK();admin_send(fd,"200 OK","application/json",out);return;}
 int id=0;
 if(sscanf(path,"/api/menu/%d",&id)==1){
  ADMIN_LOCK(); menu_item_t *m=menu_find(id);
  if(!strcmp(method,"DELETE")){if(!m){ADMIN_UNLOCK();admin_send(fd,"404 Not Found","text/plain","not found");return;}int ix=(int)(m-g_menu);memmove(&g_menu[ix],&g_menu[ix+1],(g_menu_count-ix-1)*sizeof(*m));g_menu_count--;g_menu_revision++;ADMIN_UNLOCK();admin_send(fd,"200 OK","text/plain","deleted");return;}
  if(!strcmp(method,"PUT")){if(!m){ADMIN_UNLOCK();admin_send(fd,"404 Not Found","text/plain","not found");return;}int new_id=json_int(body,"id",m->id);if(new_id!=id && menu_find(new_id)){ADMIN_UNLOCK();admin_send(fd,"409 Conflict","text/plain","duplicate id");return;}m->id=new_id;json_str(body,"name",m->name,sizeof(m->name),m->name);json_str(body,"desc",m->desc,sizeof(m->desc),m->desc);m->price=json_int(body,"price",m->price);m->category=json_int(body,"category",m->category);m->stock=json_int(body,"stock",m->stock);g_menu_revision++;ADMIN_UNLOCK();admin_send(fd,"200 OK","text/plain","updated");return;}ADMIN_UNLOCK();}
 if(!strcmp(method,"POST")&&!strcmp(path,"/api/menu")){ADMIN_LOCK();if(g_menu_count>=MENU_MAX){ADMIN_UNLOCK();admin_send(fd,"409 Conflict","text/plain","menu full");return;}int new_id=json_int(body,"id",0);if(new_id<=0||menu_find(new_id)){ADMIN_UNLOCK();admin_send(fd,"409 Conflict","text/plain","invalid or duplicate id");return;}menu_item_t*m=&g_menu[g_menu_count++];m->id=new_id;json_str(body,"name",m->name,sizeof(m->name),"");json_str(body,"desc",m->desc,sizeof(m->desc),"");m->price=json_int(body,"price",0);m->category=json_int(body,"category",0);m->stock=json_int(body,"stock",0);g_menu_revision++;ADMIN_UNLOCK();admin_send(fd,"201 Created","text/plain","created");return;}
 admin_send(fd,"404 Not Found","text/plain","not found");
}
static void *admin_thread(void *arg){int port=*(int*)arg;SOCKET s=socket(AF_INET,SOCK_STREAM,0);int one=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(const char*)&one,sizeof(one));struct sockaddr_in a;memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons((unsigned short)port);if(bind(s,(struct sockaddr*)&a,sizeof(a))!=0||listen(s,8)!=0){printf("[admin] bind %d failed\\n",port);return NULL;}printf("[admin] web console: http://127.0.0.1:%d/\\n",port);while(1){SOCKET c=accept(s,NULL,NULL);if(c!=SOCK_ERR){admin_request(c);CLOSE_SOCK(c);}}return NULL;}
#endif

/* ================================================================
 *  主程序：select 多客户端事件循环
 * ================================================================ */
int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    if (argc > 1) port = atoi(argv[1]);

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    menu_init();
#ifndef _WIN32
    {
        static int admin_port;
        static pthread_t admin_tid;
        admin_port = port + ADMIN_PORT_OFFSET;
        if (pthread_create(&admin_tid, NULL, admin_thread, &admin_port) != 0)
            printf("[admin] web console thread start failed\n");
        else
            pthread_detach(admin_tid);
    }
#endif
    memset(g_clients, 0, sizeof(g_clients));

    SOCKET listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == SOCK_ERR) {
        printf("[server] socket() failed\n");
        return 1;
    }
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* 绑定所有网卡，板子可直连 */
    addr.sin_port = htons((unsigned short)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        printf("[server] bind(0.0.0.0:%d) failed\n", port);
        return 1;
    }
    if (listen(listen_fd, 8) != 0) {
        printf("[server] listen() failed\n");
        return 1;
    }
    printf("[server] haidi_server listening on 0.0.0.0:%d ...\n", port);

    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        SOCKET maxfd = listen_fd;
        FD_SET(listen_fd, &rfds);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (g_clients[i].active) {
                FD_SET(g_clients[i].fd, &rfds);
                if (g_clients[i].fd > maxfd) maxfd = g_clients[i].fd;
            }
        }

#ifdef _WIN32
        if (select(0, &rfds, NULL, NULL, NULL) == SOCKET_ERROR) break;
#else
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) break;
#endif

        /* 新连接 */
        if (FD_ISSET(listen_fd, &rfds)) {
            struct sockaddr_in cli;
            socklen_t clen = sizeof(cli);
            SOCKET cfd = accept(listen_fd, (struct sockaddr *)&cli, &clen);
            if (cfd != SOCK_ERR) {
                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; i++)
                    if (!g_clients[i].active) { slot = i; break; }
                if (slot < 0) {
                    send_line(cfd, "ERR server_full");
                    CLOSE_SOCK(cfd);
                } else {
                    g_clients[slot].fd = cfd;
                    g_clients[slot].active = 1;
                    g_clients[slot].len = 0;
                    send_line(cfd, "WELCOME haidi_server_v1");
                    printf("[server] client connected from %s (slot %d)\n",
                           inet_ntoa(cli.sin_addr), slot);
                }
            }
        }

        /* 各客户端数据 */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            client_t *c = &g_clients[i];
            if (!c->active || !FD_ISSET(c->fd, &rfds)) continue;

            char tmp[RECV_BUF];
            int n = recv(c->fd, tmp, sizeof(tmp) - 1, 0);
            if (n <= 0) {
                printf("[server] client slot %d disconnected\n", i);
                CLOSE_SOCK(c->fd);
                c->active = 0;
                continue;
            }

            /* 拼进会话缓冲，按行处理 */
            if (c->len + n >= (int)sizeof(c->buf)) {
                c->len = 0;   /* 缓冲溢出，丢弃 */
            }
            memcpy(c->buf + c->len, tmp, n);
            c->len += n;

            int start = 0;
            for (int j = 0; j < c->len; j++) {
                if (c->buf[j] == '\n') {
                    c->buf[j] = '\0';
                    process_line(c->fd, c->buf + start);
                    start = j + 1;
                }
            }
            if (start > 0) {
                c->len -= start;
                memmove(c->buf, c->buf + start, c->len);
            }
        }
    }

    CLOSE_SOCK(listen_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
