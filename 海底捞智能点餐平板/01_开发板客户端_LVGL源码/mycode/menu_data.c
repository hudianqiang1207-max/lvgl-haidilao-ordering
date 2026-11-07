#include "menu_data.h"
#include <string.h>
#include <stdio.h>

#include "ui.h"
#include "net_client.h"

const char *menu_category_names[CAT_COUNT] = {
    "锅底", "招牌精选", "肉类", "海鲜", "蔬菜", "主食小吃", "饮品"
};

static menu_item_t g_menu[MENU_MAX_ITEMS];
static int g_menu_count = 0;
static unsigned long g_menu_version = 0;

static cart_entry_t g_cart[CART_MAX_ITEMS];
static int g_cart_count = 0;

static int cart_find(int menu_id);   /* 前向声明：pot_select 先于实现使用 */

/* 根据菜品 id 选择真实图片资源（来自海底捞官网 OSS 的 80x80 图） */
static const void *img_for_id(int id)
{
    switch (id) {
    /* 锅底 */
    case 368: return &ui_img_real_368;
    case 3307: return &ui_img_real_3307;
    case 3493: return &ui_img_real_3493;
    case 3494: return &ui_img_real_3494;
    case 3495: return &ui_img_real_3495;
    case 7634: return &ui_img_real_7634;
    case 7676: return &ui_img_real_7676;
    /* 招牌/特色 */
    case 428: return &ui_img_real_428;
    case 3510: return &ui_img_real_3510;
    case 8267: return &ui_img_real_8267;
    case 8268: return &ui_img_real_8268;
    case 8269: return &ui_img_real_8269;
    case 8270: return &ui_img_real_8270;
    case 8271: return &ui_img_real_8271;
    case 8272: return &ui_img_real_8272;
    case 8273: return &ui_img_real_8273;
    case 8679: return &ui_img_real_8679;
    /* 肉类 */
    case 8306: return &ui_img_real_8306;
    case 8307: return &ui_img_real_8307;
    case 8308: return &ui_img_real_8308;
    case 8309: return &ui_img_real_8309;
    case 8310: return &ui_img_real_8310;
    case 8311: return &ui_img_real_8311;
    case 8312: return &ui_img_real_8312;
    case 8313: return &ui_img_real_8313;
    case 8314: return &ui_img_real_8314;
    case 8315: return &ui_img_real_8315;
    case 8316: return &ui_img_real_8316;
    case 8317: return &ui_img_real_8317;
    case 8318: return &ui_img_real_8318;
    case 8319: return &ui_img_real_8319;
    case 8320: return &ui_img_real_8320;
    case 8499: return &ui_img_real_8499;
    case 8502: return &ui_img_real_8502;
    case 8503: return &ui_img_real_8503;
    case 8504: return &ui_img_real_8504;
    /* 新品 */
    case 8498: return &ui_img_real_8498;
    case 8501: return &ui_img_real_8501;
    case 8674: return &ui_img_real_8674;
    case 8675: return &ui_img_real_8675;
    case 8676: return &ui_img_real_8676;
    case 8677: return &ui_img_real_8677;
    case 8678: return &ui_img_real_8678;
    /* 小吃 */
    case 130: return &ui_img_real_130;
    case 3604: return &ui_img_real_3604;
    case 3608: return &ui_img_real_3608;
    case 8321: return &ui_img_real_8321;
    case 8322: return &ui_img_real_8322;
    case 8525: return &ui_img_real_8525;
    case 8526: return &ui_img_real_8526;
    /* 蘸料 */
    case 456: return &ui_img_real_456;
    case 3325: return &ui_img_real_3325;
    case 3594: return &ui_img_real_3594;
    case 3595: return &ui_img_real_3595;
    case 3596: return &ui_img_real_3596;
    case 3597: return &ui_img_real_3597;
    case 3598: return &ui_img_real_3598;
    case 3599: return &ui_img_real_3599;
    /* 饮品 */
    case 21: return &ui_img_real_21;   /* 酸梅汤（用户实物图） */
    case 40: return &ui_img_real_40;
    case 3618: return &ui_img_real_3618;
    case 3762: return &ui_img_real_3762;
    default: return &ui_img_real_428;
    }
}

/* 本地默认菜单（离线兜底，与服务器数据一致） */
static void load_local_menu(void)
{
    g_menu_count = 0;
    memset(g_menu, 0, sizeof(g_menu));

#define ADD_MENU(ID, NAME, DESC, PRICE, CAT, STOCK) do { \
    menu_item_t *m = &g_menu[g_menu_count++]; \
    m->id = ID; strcpy(m->name, NAME); strcpy(m->desc, DESC); \
    m->price = PRICE; m->category = CAT; m->stock = STOCK; \
    m->img_src = img_for_id(ID); \
} while (0)

    /* ---- 锅底（真实海底捞锅底） ---- */
    ADD_MENU(368,  "新一代番茄火锅", "一定要喝汤的番茄火锅", 68, CAT_POT, 20);
    ADD_MENU(3307, "经典麻辣火锅",   "慢火熬煮4小时的经典麻辣锅底", 88, CAT_POT, 20);
    ADD_MENU(3493, "清油麻辣火锅",   "植物油炒制，香辣浓郁不油腻", 78, CAT_POT, 20);
    ADD_MENU(3494, "三鲜火锅",       "色泽纯正，口感鲜香浓郁", 58, CAT_POT, 20);
    ADD_MENU(3495, "岭南黄猪肚鸡火锅", "汤头乳白、鲜香浓郁", 98, CAT_POT, 15);
    ADD_MENU(7634, "浓浓浓菌汤锅",   "研发300天，菌汤超级鲜", 62, CAT_POT, 20);
    ADD_MENU(7676, "真香锅",         "汤底鲜香，麻辣适中，清爽不油腻", 82, CAT_POT, 15);

    /* ---- 招牌精选（特色菜） ---- */
    ADD_MENU(428,  "捞派捞面",       "明星产品，现场捞面表演", 12, CAT_RECOMMEND, 20);
    ADD_MENU(3510, "捞派脆脆毛肚",   "毛肚即牛胃，涮8秒最脆", 58, CAT_RECOMMEND, 10);
    ADD_MENU(8267, "捞派鸭肠",       "每一根都口感爽脆有韧性", 42, CAT_RECOMMEND, 10);
    ADD_MENU(8268, "捞派豆花",       "传统手工工艺压制，嫩滑", 18, CAT_RECOMMEND, 15);
    ADD_MENU(8269, "捞派麻辣滑牛肉", "精选牛小米龙部位，嫩滑", 62, CAT_RECOMMEND, 10);
    ADD_MENU(8270, "招牌大颗粒虾滑", "虾肉含量高，鲜甜Q弹", 52, CAT_RECOMMEND, 12);
    ADD_MENU(8271, "捞派嫩羊肉",     "优质羊后腿肉，口感嫩滑", 55, CAT_RECOMMEND, 10);
    ADD_MENU(8272, "捞派黄喉",       "精选猪黄喉，去筋膜去腥", 46, CAT_RECOMMEND, 10);
    ADD_MENU(8273, "捞派巴沙鱼片",   "越南巴沙鱼，少刺鲜嫩", 38, CAT_RECOMMEND, 12);
    ADD_MENU(8679, "8秒海带苗",      "8秒涮烫，一口脆爽", 16, CAT_RECOMMEND, 15);

    /* ---- 肉类 ---- */
    ADD_MENU(8306, "捞派肥牛",       "进口谷饲牛肉，肥瘦相间", 58, CAT_MEAT, 12);
    ADD_MENU(8307, "精品腹肉肥牛",   "肥瘦相间，口感醇厚", 62, CAT_MEAT, 10);
    ADD_MENU(8308, "精选牛板腱",     "雪花丰富，肉香浓郁", 66, CAT_MEAT, 10);
    ADD_MENU(8309, "新西兰羔羊羊排卷", "10个月小羔羊，肉中贵族", 68, CAT_MEAT, 8);
    ADD_MENU(8310, "内蒙特色沙葱羊肉卷", "草原羔羊+沙葱清香", 60, CAT_MEAT, 10);
    ADD_MENU(8311, "精品肥羊1号",    "前肩部位肉，肥瘦搭配", 56, CAT_MEAT, 10);
    ADD_MENU(8312, "层层雪浪猪五花", "肥瘦相间、香而不腻", 45, CAT_MEAT, 12);
    ADD_MENU(8313, "火锅牛排",       "牛板腱部位，鲜嫩多汁", 72, CAT_MEAT, 8);
    ADD_MENU(8314, "我的小郡肝",     "鸡胗切花，爽脆咸鲜麻辣", 32, CAT_MEAT, 15);
    ADD_MENU(8315, "鲜切羊肉",       "冷鲜腿肉，瘦而不硬", 52, CAT_MEAT, 10);
    ADD_MENU(8316, "脱骨鸭掌",       "剔硬骨保留完整性，劲道", 35, CAT_MEAT, 12);
    ADD_MENU(8317, "猪脑花",         "精选优质猪脑，细腻嫩滑", 28, CAT_MEAT, 8);
    ADD_MENU(8318, "鸭舌",           "优质鸭舌，肉嫩鲜美", 38, CAT_MEAT, 10);
    ADD_MENU(8319, "脆皮肠",         "优质鸡肉，爽脆烟熏味", 22, CAT_MEAT, 15);
    ADD_MENU(8320, "午餐肉",         "精选猪后腿肉，肉香浓郁", 25, CAT_MEAT, 15);
    ADD_MENU(8499, "精选安格斯牛板腱", "连筋带肉，有嚼劲", 78, CAT_MEAT, 8);
    ADD_MENU(8502, "臻选安格斯牛板腱", "只用安格斯牛，肉嫩多汁", 88, CAT_MEAT, 6);
    ADD_MENU(8503, "经典连筋带肉牛板腱", "板腱取牛肩胛部位", 75, CAT_MEAT, 8);
    ADD_MENU(8504, "川式香辣滑牛肉", "裹满辣椒面，又香又辣", 55, CAT_MEAT, 10);

    /* ---- 海鲜（新品+海鲜类） ---- */
    ADD_MENU(8498, "莓莓抹茶牛乳",   "抹茶+草莓果酱，惊喜口感", 22, CAT_SEAFOOD, 12);
    ADD_MENU(8501, "雪花肩胛小排",   "牛肋骨3-5根之间，肉香足", 68, CAT_SEAFOOD, 8);
    ADD_MENU(8674, "内蒙罕山白绒山羊", "赤峰羊肉，地理标志产品", 66, CAT_SEAFOOD, 8);
    ADD_MENU(8675, "虫草花乌鸡丸",   "乌鸡虫草花，营养高蛋白", 42, CAT_SEAFOOD, 10);
    ADD_MENU(8676, "雪人豆乳蛋糕",   "有颜有料，双重口感", 18, CAT_SEAFOOD, 15);
    ADD_MENU(8677, "虾滑鱼籽三明治", "三层口感三层鲜", 48, CAT_SEAFOOD, 10);
    ADD_MENU(8678, "黑松露鱼籽牛肉饭", "鱼籽松露香，一口香迷糊", 58, CAT_SEAFOOD, 8);

    /* ---- 蔬菜（小吃甜品） ---- */
    ADD_MENU(130,  "现炸酥肉",       "酥脆可口，椒香口味", 28, CAT_VEG, 15);
    ADD_MENU(3604, "茴香小油条",     "鲜茴香，外酥里软", 12, CAT_VEG, 20);
    ADD_MENU(3608, "五常米饭",       "米香浓郁，软糯可口", 3, CAT_VEG, 50);
    ADD_MENU(8321, "一口爆浆奶酪鱼条", "外酥里嫩，奶酪味十足", 32, CAT_VEG, 12);
    ADD_MENU(8322, "酥酥嫩嫩炸牛奶", "口感酥脆，老少皆宜", 18, CAT_VEG, 15);
    ADD_MENU(8525, "你龙我龙慕斯(白桃)", "清甜可口，桃香四溢", 22, CAT_VEG, 12);
    ADD_MENU(8526, "喜乐龙龙慕斯(蜜瓜)", "蜜瓜清新，口感细腻", 22, CAT_VEG, 12);

    /* ---- 主食小吃（蘸料） ---- */
    ADD_MENU(456,  "特色蘸料",       "锅底百搭", 6, CAT_STAPLE, 30);
    ADD_MENU(3325, "鲜香味碟",       "提鲜必备", 6, CAT_STAPLE, 30);
    ADD_MENU(3594, "香辣味碟",       "川渝搭档", 6, CAT_STAPLE, 30);
    ADD_MENU(3595, "海鲜味碟",       "海鲜专属", 6, CAT_STAPLE, 30);
    ADD_MENU(3596, "丸滑味碟",       "丸滑必备", 6, CAT_STAPLE, 30);
    ADD_MENU(3597, "酸辣味碟",       "酸辣开胃", 6, CAT_STAPLE, 30);
    ADD_MENU(3598, "蒜泥香油碟",     "川式口味", 6, CAT_STAPLE, 30);
    ADD_MENU(3599, "芝麻调味酱",     "老北京口味", 6, CAT_STAPLE, 30);

    /* ---- 饮品 ---- */
    ADD_MENU(40,   "海底捞大麦拉格啤酒", "天禄奖一星，甄选大麦芽", 18, CAT_DRINK, 12);
    ADD_MENU(3618, "海底捞德式小麦啤酒", "CCBA银奖，果香馥郁", 18, CAT_DRINK, 12);
    ADD_MENU(3762, "海底捞淡爽拉格啤酒", "CCBA铜奖，酒体金黄", 16, CAT_DRINK, 12);
    ADD_MENU(21,   "酸梅汤",         "自制酸梅·冰爽解腻", 15, CAT_DRINK, 12);
#undef ADD_MENU
}

void menu_data_init(void)
{
    /* 1. 本地默认数据（离线兜底） */
    load_local_menu();

    /* 2. 尝试连接 PC/云服务器，用服务器菜单+库存覆盖本地 */
    net_init();
    if (net_connect_server() == 0) {
        if (menu_load_from_server() == 0) {
            net_get_menu_version(&g_menu_version);
            printf("菜单数据已从服务器同步 (%d 道)\n", g_menu_count);
        } else {
            printf("服务器菜单解析失败，使用本地数据\n");
        }
    } else {
        printf("服务器连接失败(%s:%d)，使用本地数据\n", SERVER_IP, SERVER_PORT);
    }

    cart_clear();
    printf("菜单数据加载完成: %d 道菜品\n", g_menu_count);
}

/* 从服务器拉取菜单（GET_MENU），成功返回 0 */
int menu_load_from_server(void)
{
    menu_item_t new_menu[MENU_MAX_ITEMS];
    int new_count = 0;
    if (net_send_line("GET_MENU") != 0) return -1;

    char line[128];
    if (net_recv_line(line, sizeof(line), 2000) != 0) return -1;

    int n = 0;
    if (sscanf(line, "MENU %d", &n) != 1 || n <= 0 || n > MENU_MAX_ITEMS) return -1;

    for (int i = 0; i < n; i++) {
        if (net_recv_line(line, sizeof(line), 2000) != 0) return -1;
        int id = 0, price = 0, cat = 0, stock = 0;
        char name[MENU_MAX_NAME] = {0};
        char desc[MENU_MAX_DESC] = {0};
        if (sscanf(line, "ITEM %d|%31[^|]|%47[^|]|%d|%d|%d",
                   &id, name, desc, &price, &cat, &stock) == 6) {
            menu_item_t *m = &new_menu[new_count++];
            m->id = id;
            snprintf(m->name, MENU_MAX_NAME, "%s", name);
            snprintf(m->desc, MENU_MAX_DESC, "%s", desc);
            m->price = price;
            m->category = cat;
            m->stock = stock;
            m->img_src = img_for_id(id);
        }
    }
    if (new_count != n || new_count <= 0) return -1;
    memcpy(g_menu, new_menu, sizeof(new_menu));
    g_menu_count = new_count;
    return 0;
}

int menu_sync_if_changed(void)
{
    unsigned long remote = 0;
    if (!net_is_connected() || net_get_menu_version(&remote) != 0) return -1;
    if (g_menu_version != 0 && remote == g_menu_version) return 0;
    if (menu_load_from_server() != 0) return -1;
    g_menu_version = remote;
    return 1;
}

int menu_get_count(int category)
{
    int count = 0;
    for (int i = 0; i < g_menu_count; i++) {
        if (g_menu[i].category == category) count++;
    }
    return count;
}

const menu_item_t *menu_get_item(int category, int index)
{
    int n = 0;
    for (int i = 0; i < g_menu_count; i++) {
        if (g_menu[i].category == category) {
            if (n == index) return &g_menu[i];
            n++;
        }
    }
    return NULL;
}

const menu_item_t *menu_find_by_id(int id)
{
    for (int i = 0; i < g_menu_count; i++) {
        if (g_menu[i].id == id) return &g_menu[i];
    }
    return NULL;
}

int menu_get_stock(int id)
{
    const menu_item_t *m = menu_find_by_id(id);
    return m ? m->stock : 0;
}

void menu_set_stock(int id, int stock)
{
    menu_item_t *m = (menu_item_t *)menu_find_by_id(id);
    if (m) m->stock = stock;
}

/* ---- cart ---- */

/* 当前选中的锅底 id（0 = 未选） */
static int g_selected_pot = 0;

/* 选择锅底（单选，点击锅底卡片时调用）。
 * 锅底计入订单：换锅底时先移除旧锅底（购物车数量>1 时清到 0），再加入新锅底（qty=1）。 */
void pot_select(int id)
{
    if (id == g_selected_pot) return;   /* 重复点击同一锅底忽略 */

    /* 移除旧锅底（可能被加购过，清到 0 条） */
    if (g_selected_pot != 0) {
        while (cart_find(g_selected_pot) >= 0)
            cart_remove(g_selected_pot);
    }

    g_selected_pot = id;
    if (id != 0) {
        /* 新锅底加入购物车（若已在购物车则保证 qty=1） */
        int idx = cart_find(id);
        if (idx >= 0) {
            g_cart[idx].quantity = 1;
        } else {
            cart_add(id);
        }
    }
    printf("[menu] 已选择锅底: %s\n", pot_get_selected_name());
}

/* 返回当前选中的锅底 id，0 表示未选 */
int pot_get_selected(void)
{
    return g_selected_pot;
}

/* 返回当前选中锅底的名称（未选返回空串） */
const char *pot_get_selected_name(void)
{
    if (g_selected_pot == 0) return "";
    const menu_item_t *m = menu_find_by_id(g_selected_pot);
    return m ? m->name : "";
}

void cart_clear(void)
{
    g_cart_count = 0;
    memset(g_cart, 0, sizeof(g_cart));
}

void cart_add(int menu_id)
{
    for (int i = 0; i < g_cart_count; i++) {
        if (g_cart[i].menu_id == menu_id) {
            g_cart[i].quantity++;
            return;
        }
    }
    if (g_cart_count < CART_MAX_ITEMS) {
        g_cart[g_cart_count].menu_id = menu_id;
        g_cart[g_cart_count].quantity = 1;
        g_cart_count++;
    }
}

void cart_remove(int menu_id)
{
    for (int i = 0; i < g_cart_count; i++) {
        if (g_cart[i].menu_id == menu_id) {
            if (g_cart[i].quantity > 1) {
                g_cart[i].quantity--;
            } else {
                memmove(&g_cart[i], &g_cart[i + 1],
                        (g_cart_count - i - 1) * sizeof(cart_entry_t));
                g_cart_count--;
            }
            return;
        }
    }
}

/* 返回购物车中该菜品的下标，-1 表示不在购物车 */
static int cart_find(int menu_id)
{
    for (int i = 0; i < g_cart_count; i++)
        if (g_cart[i].menu_id == menu_id) return i;
    return -1;
}

int cart_get_count(void)
{
    int total = 0;
    for (int i = 0; i < g_cart_count; i++) total += g_cart[i].quantity;
    return total;
}

int cart_get_total(void)
{
    int sum = 0;
    for (int i = 0; i < g_cart_count; i++) {
        const menu_item_t *m = menu_find_by_id(g_cart[i].menu_id);
        if (m) sum += m->price * g_cart[i].quantity;
    }
    return sum;
}

int cart_get_entry_count(void)
{
    return g_cart_count;
}

const cart_entry_t *cart_get_entry(int index)
{
    if (index < 0 || index >= g_cart_count) return NULL;
    return &g_cart[index];
}
