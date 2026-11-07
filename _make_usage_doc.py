from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
out=r'E:\AI-Workspace\projects\海底捞点餐平板\《海底捞智能点餐系统使用文档》.docx'
d=Document(); s=d.sections[0]; s.top_margin=Cm(2); s.bottom_margin=Cm(2); s.left_margin=Cm(2.3); s.right_margin=Cm(2.3)
for n,z in [('Normal',10.5),('Title',24),('Heading 1',16),('Heading 2',12.5)]:
 st=d.styles[n]; st.font.name='Microsoft YaHei'; st._element.rPr.rFonts.set(qn('w:eastAsia'),'微软雅黑'); st.font.size=Pt(z)
d.styles['Title'].font.color.rgb=RGBColor(201,21,30); d.styles['Heading 1'].font.color.rgb=RGBColor(201,21,30)
def p(t='',center=False,bold=False,size=None):
 x=d.add_paragraph(); x.alignment=WD_ALIGN_PARAGRAPH.CENTER if center else WD_ALIGN_PARAGRAPH.LEFT; x.paragraph_format.space_after=Pt(6); r=x.add_run(t); r.bold=bold
 if size:r.font.size=Pt(size)
 return x
def b(t): d.add_paragraph(t,style='List Bullet')
def tab(h,rows):
 t=d.add_table(rows=1,cols=len(h)); t.style='Table Grid'; t.alignment=WD_TABLE_ALIGNMENT.CENTER
 for i,x in enumerate(h):
  c=t.rows[0].cells[i]; c.text=x; sh=OxmlElement('w:shd'); sh.set(qn('w:fill'),'C9151E'); c._tc.get_or_add_tcPr().append(sh); c.paragraphs[0].runs[0].font.color.rgb=RGBColor(255,255,255); c.paragraphs[0].runs[0].bold=True
 for row in rows:
  cs=t.add_row().cells
  for i,x in enumerate(row):cs[i].text=str(x)
 d.add_paragraph()
p('海底捞智能点餐系统',True,True,25); p('系统使用文档',True,True,20); p('适用于开发板点餐端和 Web 管理端',True,False,11); d.add_page_break()
d.add_heading('一、系统简介',1); p('本系统由 GEC6818 开发板点餐端、云服务器和浏览器管理端组成。顾客使用开发板触摸屏完成选锅底、选菜、购物车管理、结算和模拟出货；管理人员使用 Web 页面维护菜单、库存和订单。')
d.add_heading('二、系统地址',1); tab(['项目','地址或说明'],[('开发板服务器','8.138.82.185:8888'),('浏览器管理端','http://8.138.82.185:8889/'),('默认菜单','62 道菜品')])
d.add_heading('三、开发板使用方法',1); d.add_heading('1. 启动系统',2); p('开发板接通电源后，在终端执行：'); p('cd /root\nchmod +x haidi_arm\n./haidi_arm',False,False,10)
d.add_heading('2. 选择锅底',2); p('在点餐页面选择番茄、麻辣、三鲜或菌汤等锅底，选中后顶部会显示当前锅底。')
d.add_heading('3. 选择菜品',2); p('点击左侧分类浏览菜品，点击商品卡片上的“+”加入购物车。库存为 0 时显示“售罄”，不能继续添加。')
d.add_heading('4. 管理购物车',2); b('点击“+”增加数量。'); b('点击“−”减少数量。'); b('购物车显示商品数量和总金额。')
d.add_heading('5. 提交订单',2); p('点击结算，确认订单后选择微信支付、支付宝支付或现金支付。当前支付为演示模拟，不连接真实支付平台。支付成功后进入模拟出货页面。')
d.add_heading('四、管理端使用方法',1); p('浏览器访问：'); p('http://8.138.82.185:8889/',False,True,11); p('页面显示菜单列表后，可以查看 ID、名称、描述、价格、分类和库存。')
d.add_heading('1. 修改菜品',2); p('点击目标菜品“编辑”，修改表单内容后点击“新增/修改”。可修改名称、描述、价格、分类和库存。')
d.add_heading('2. 新增菜品',2); p('填写未使用的 ID、名称、描述、价格、分类和库存，然后点击“新增/修改”。建议临时测试使用 ID 9999，测试完成后删除。')
d.add_heading('3. 删除菜品',2); p('点击目标菜品的“删除”并确认。删除后开发板会在下一次菜单同步时移除该商品。')
d.add_heading('4. 查看订单',2); p('点击“查看订单”，可以查看订单号、菜品名称、数量、单价、小计、总金额和订单状态。')
d.add_heading('五、实时同步说明',1); p('管理端保存菜单后，服务器更新菜单版本。开发板约每 5 秒检查一次版本；发现变化后重新下载完整菜单。因此名称、价格、分类、库存、新增和删除商品都会同步到开发板。')
p('正常同步日志：'); p('[menu] 周期同步完成，版本=xxxxx，商品=62\n[menu] 后台同步完成，当前分类已刷新',False,False,9)
d.add_heading('六、常见问题',1); tab(['问题','处理方法'],[('页面没有更新','浏览器按 Ctrl+F5 强制刷新。'),('开发板没有同步','等待约 5 秒，确认终端出现“周期同步完成”。'),('仍显示旧菜单','重启开发板程序，并确认连接的是 8.138.82.185:8888。'),('服务器端口占用','只保留一个 haidi_server 进程。'),('商品售罄','在管理端把库存改为大于 0，等待下一次同步。')])
d.add_heading('七、使用注意事项',1); b('开发板连接 TCP 端口 8888，浏览器管理端使用 8889。'); b('不要同时启动多个服务器进程。'); b('测试新增商品完成后删除，保持默认菜单为 62 道。'); b('当前支付和出货为演示功能，不代表真实支付和自动出餐设备。')
d.add_heading('八、演示建议',1); p('建议依次演示：启动开发板 → 选择锅底 → 添加菜品 → 修改购物车 → 结算支付 → 查看管理台 → 修改价格 → 修改库存为 0 → 展示开发板出现“售罄”。')
for sec in d.sections:
 f=sec.footer.paragraphs[0]; f.alignment=WD_ALIGN_PARAGRAPH.CENTER; f.add_run('海底捞智能点餐系统使用文档').font.size=Pt(9)
d.save(out); print(out)
