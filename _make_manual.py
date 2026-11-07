from docx import Document
from docx.shared import Cm, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.section import WD_SECTION
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.enum.style import WD_STYLE_TYPE
from docx.enum.text import WD_BREAK

out = r'E:\AI-Workspace\projects\海底捞点餐平板\《海底捞智能点餐系统使用说明书》.docx'
doc = Document()
sec = doc.sections[0]
sec.top_margin = Cm(2.2); sec.bottom_margin = Cm(2.0); sec.left_margin = Cm(2.4); sec.right_margin = Cm(2.4)
styles = doc.styles
styles['Normal'].font.name = 'Microsoft YaHei'; styles['Normal']._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑'); styles['Normal'].font.size = Pt(10.5)
for sname, size, color in [('Title', 24, 'C9151E'), ('Heading 1', 16, 'C9151E'), ('Heading 2', 12.5, '333333')]:
    st=styles[sname]; st.font.name='Microsoft YaHei'; st._element.rPr.rFonts.set(qn('w:eastAsia'),'微软雅黑'); st.font.size=Pt(size); st.font.bold=True; st.font.color.rgb=RGBColor.from_string(color)

def shade(cell, fill):
    tcPr=cell._tc.get_or_add_tcPr(); shd=OxmlElement('w:shd'); shd.set(qn('w:fill'),fill); tcPr.append(shd)
def set_cell_text(cell, text, bold=False, color=None):
    cell.text=''; p=cell.paragraphs[0]; p.paragraph_format.space_after=Pt(0); r=p.add_run(str(text)); r.bold=bold
    if color: r.font.color.rgb=RGBColor.from_string(color)
    cell.vertical_alignment=WD_CELL_VERTICAL_ALIGNMENT.CENTER
def table(headers, rows, widths=None):
    t=doc.add_table(rows=1, cols=len(headers)); t.alignment=WD_TABLE_ALIGNMENT.CENTER; t.style='Table Grid'
    for i,h in enumerate(headers): set_cell_text(t.rows[0].cells[i],h,True,'FFFFFF'); shade(t.rows[0].cells[i],'C9151E')
    for row in rows:
        cells=t.add_row().cells
        for i,v in enumerate(row): set_cell_text(cells[i],v)
    doc.add_paragraph().paragraph_format.space_after=Pt(1)
    return t
def para(text='', bold=False, center=False, size=None, color=None):
    p=doc.add_paragraph(); p.alignment=WD_ALIGN_PARAGRAPH.CENTER if center else WD_ALIGN_PARAGRAPH.LEFT; p.paragraph_format.space_after=Pt(6); p.paragraph_format.line_spacing=1.25
    r=p.add_run(text); r.bold=bold
    if size: r.font.size=Pt(size)
    if color: r.font.color.rgb=RGBColor.from_string(color)
    return p
def bullet(text):
    p=doc.add_paragraph(style='List Bullet'); p.paragraph_format.space_after=Pt(3); p.add_run(text); return p

def code(text):
    p=doc.add_paragraph(); p.paragraph_format.left_indent=Cm(0.5); p.paragraph_format.space_after=Pt(6)
    r=p.add_run(text); r.font.name='Consolas'; r._element.rPr.rFonts.set(qn('w:eastAsia'),'等线'); r.font.size=Pt(9); r.font.color.rgb=RGBColor.from_string('444444'); return p

# cover
p=doc.add_paragraph(); p.alignment=WD_ALIGN_PARAGRAPH.CENTER; p.paragraph_format.space_before=Pt(100)
r=p.add_run('海底捞智能点餐系统'); r.bold=True; r.font.size=Pt(28); r.font.color.rgb=RGBColor.from_string('C9151E')
p=doc.add_paragraph(); p.alignment=WD_ALIGN_PARAGRAPH.CENTER; r=p.add_run('使用说明书'); r.bold=True; r.font.size=Pt(22); r.font.color.rgb=RGBColor.from_string('333333')
para('基于 GEC6818 + LVGL + TCP/HTTP 的嵌入式智能点餐系统', center=True, size=12, color='666666')
para('', center=True)
para('版本：V1.0', center=True, size=11)
para('适用对象：项目演示、课程答辩与系统使用人员', center=True, size=11)
doc.add_page_break()

# contents-like
para('文档说明', bold=True, size=16, color='C9151E')
para('本文档用于说明海底捞智能点餐系统的组成、启动方法、点餐操作、后台管理、实时同步机制和常见问题。系统包含开发板点餐端、云服务器和浏览器管理台三部分。')

doc.add_heading('1. 项目概述', level=1)
para('海底捞智能点餐系统是一套面向餐饮场景的嵌入式点餐解决方案。顾客通过 GEC6818 开发板触摸屏选择锅底和菜品、调整数量并提交订单；服务器负责菜单、库存和订单处理；管理人员通过 Web 管理台维护菜单并查看订单。')
bullet('开发板端：LVGL 图形界面、中文字体、触摸点餐、购物车、支付与出货演示。')
bullet('服务器端：TCP 菜单与订单服务、HTTP 管理台、菜单版本控制和库存校验。')
bullet('管理端：浏览器查看和编辑菜品，支持新增、修改、删除、库存维护和订单明细查看。')

doc.add_heading('2. 系统组成与环境', level=1)
table(['组成','说明'],[
('开发板','GEC6818，ARM Linux，800×480 LCD，触摸设备 /dev/input/event0，帧缓冲 /dev/fb0'),
('客户端','LVGL 9，FreeType 中文字体 simkai.ttf，ARM 可执行程序 haidi_arm'),
('云服务器','Ubuntu Linux，运行 haidi_server'),
('TCP 服务','8.138.82.185:8888，用于开发板菜单、订单、支付和出货通信'),
('Web 管理台','http://8.138.82.185:8889/，用于菜单和订单管理'),
('默认菜单','62 道菜品；测试新增商品完成后应删除临时商品')])

doc.add_heading('3. 系统启动', level=1)
doc.add_heading('3.1 启动云服务器', level=2)
para('在云服务器终端执行以下命令。服务器只应启动一个实例，否则会出现端口占用。')
code('cd /root\\n')
para('正常日志应包含：')
code('[server] menu loaded: 62 items\\n')
para('管理台端口为 TCP 端口加 1，即 8889。可使用以下命令检查：')
code("ss -lntp | grep -E '8888|8889'")

doc.add_heading('3.2 启动开发板客户端', level=2)
para('登录开发板后执行：')
code('cd /root\\n')
para('正常启动日志应包含：')
code('[net] 已连接服务器 8.138.82.185:8888\\n')

doc.add_heading('4. 顾客端使用方法', level=1)
doc.add_heading('4.1 选择锅底', level=2)
para('进入点餐页面后，在锅底区域选择需要的锅底，例如番茄火锅、经典麻辣火锅、三鲜火锅或菌汤锅。顶部会显示当前选中的锅底。')
doc.add_heading('4.2 浏览分类和添加菜品', level=2)
para('点击左侧分类按钮浏览商品。系统按照菜单分类展示菜品，点击商品卡片右侧的“+”按钮即可加入购物车。库存为 0 的商品显示“售罄”，不可继续添加。')
doc.add_heading('4.3 修改购物车', level=2)
bullet('点击“+”增加商品数量。')
bullet('点击“−”减少商品数量。')
bullet('数量减少到 0 后，商品从购物车移除。')
bullet('购物车实时显示商品数量和订单总金额。')
doc.add_heading('4.4 结算、支付与出货', level=2)
para('点击结算按钮确认订单，选择微信支付、支付宝支付或现金支付。当前项目中的支付为演示模拟流程，不连接真实支付平台。支付成功后进入出货页面，系统模拟自动出餐。')

doc.add_heading('5. Web 管理台使用方法', level=1)
para('在浏览器中访问：')
code('http://8.138.82.185:8889/')
para('如果页面没有显示最新内容，请按 Ctrl+F5 强制刷新。')
doc.add_heading('5.1 菜单管理', level=2)
table(['操作','步骤','结果'],[
('查看菜单','点击“刷新菜单”','显示当前菜单、价格、分类和库存'),
('编辑菜品','点击目标菜品的“编辑”，修改表单后点击“新增/修改”','更新名称、描述、价格、分类或库存'),
('新增菜品','填写未使用的 ID、名称、描述、价格、分类和库存后保存','新增商品加入服务器菜单'),
('删除菜品','点击对应行的“删除”并确认','商品从服务器菜单中移除')])
para('分类编号以管理台提示为准。临时测试商品建议使用 ID 9999，测试结束后删除，确保默认菜单保持 62 道。')
doc.add_heading('5.2 订单查看', level=2)
para('点击“查看订单”，可查看订单号、菜品明细、数量、单价、小计、订单总金额和订单状态。订单状态包括待支付、已支付和已出货。')

doc.add_heading('6. 实时同步机制', level=1)
para('管理台修改菜单后，服务器会递增菜单版本号。开发板客户端约每 5 秒请求一次 MENU_VERSION；如果发现菜单数据发生变化，就通过 GET_MENU 下载完整菜单并刷新当前界面。同步内容包括：')
bullet('菜品名称和描述')
bullet('菜品价格')
bullet('菜品分类')
bullet('库存数量')
bullet('新增和删除的商品')
para('因此管理台修改价格或库存后，开发板不只更新库存，而是重新获取完整商品记录。库存为 0 时，开发板显示“售罄”并隐藏加号按钮。正常同步日志如下：')
code('[menu] 周期同步完成，版本=xxxxx，商品=62\\n')

doc.add_heading('7. 通信协议简介', level=1)
table(['类型','命令或接口','功能'],[
('TCP','GET_MENU','获取完整菜单'),('TCP','MENU_VERSION','获取菜单版本'),('TCP','ORDER','提交订单'),('TCP','PAY','模拟支付'),('TCP','DISPENSE','模拟出货'),('TCP','STATUS','查询服务状态'),('HTTP','GET /api/menu','获取管理台菜单 JSON'),('HTTP','POST /api/menu','新增菜品'),('HTTP','PUT /api/menu/:id','修改菜品'),('HTTP','DELETE /api/menu/:id','删除菜品'),('HTTP','GET /api/orders','获取订单 JSON')])

doc.add_heading('8. 常见问题与处理', level=1)
table(['现象','原因与处理'],[
('开发板无法连接服务器','确认云服务器已启动，并检查 8.138.82.185:8888；开发板不能连接 8889。'),
('服务器提示 bind failed','已有旧服务占用端口。执行 killall haidi_server 后只启动一个实例。'),
('开发板显示本地菜单','查看启动日志是否出现“服务器菜单解析失败”；确认已上传最新 /root/haidi_arm 并重启程序。'),
('管理台修改后没有更新','等待约 5 秒并观察 [menu] 周期同步完成；确认商品位于当前开发板分类。'),
('商品显示售罄','库存为 0 是正常状态；需要在管理台将库存改为正数后等待下一次同步。'),
('管理台页面为空白或旧数据','使用 Ctrl+F5 刷新，并确认 8889 端口由当前 haidi_server 进程监听。'),
('名称为空','在管理台编辑该商品并重新填写名称；ID 21 默认名称为“酸梅汤”。')])

doc.add_heading('9. 演示与验收建议', level=1)
para('录制演示视频时建议按以下顺序：')
for x in ['展示开发板启动和中文界面','选择锅底并添加菜品，展示购物车和总价','提交订单并完成模拟支付、出货','打开 Web 管理台，修改价格并刷新菜单','把同一商品库存改为 0，等待约 5 秒，展示开发板“售罄”','查看订单页面中的菜品名称、数量、单价、小计和总金额','结束前删除临时测试商品，恢复默认 62 道菜单']:
    bullet(x)

doc.add_heading('10. 项目总结', level=1)
para('本系统完成了嵌入式触摸点餐、中文图形界面、TCP 客户端/服务器通信、Web 菜单管理、订单管理、库存控制、模拟支付出货和菜单实时同步等功能。系统采用开发板客户端、云服务器和浏览器管理台的分层结构，具有较清晰的功能边界和扩展基础。')

# footer page numbers
for section in doc.sections:
    footer=section.footer.paragraphs[0]; footer.alignment=WD_ALIGN_PARAGRAPH.CENTER
    run=footer.add_run('海底捞智能点餐系统使用说明书  |  '); run.font.size=Pt(9); run.font.color.rgb=RGBColor.from_string('888888')
    fld=OxmlElement('w:fldSimple'); fld.set(qn('w:instr'),'PAGE'); footer._p.append(fld)

doc.save(out)
print(out)

