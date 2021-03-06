#ifndef HTTP_SERVER1_PAGE_H
#define HTTP_SERVER1_PAGE_H
#define CONFIG_PAGE "\
<!doctype html>\
<html>\
<head>\
<meta charset='utf-8'>\
<meta content='width=device-width,initial-scale=1' name='viewport'>\
<title>ESP32 Config</title>\
<style>\
body {\
font-family: Arial, Helvetica, sans-serif;\
background: #ffffff;\
color: #000000;\
font-size: 16px\
}\
h2 {\
font-size: 18px\
}\
section.main {\
margin-top: 30px;\
justify-content: center;\
display: flex\
}\
.n1 {\
height: 25px;\
}\
.n5 {\
height: 100px;\
}\
.save {\
margin-top: 50px;\
margin-right: 50px;\
height: 60px;\
width: 120px;\
font-size: large;\
}\
td {\
width: 130px;\
padding: 5px;\
horiz-align: left\
}\
.spacer {\
height: 20px;\
}\
tr {\
text-align: right\
}\
</style>\
</head>\
<body>\
<section class='main'>\
<div id='content'>\
<script>\
if (window.location.search.substr(1) !== '') {\
document.getElementById('content').display = 'none';\
document.body.innerHTML = '<h1>ESP32串口透传配置</h1>设置成功， 请等待几秒钟生效';\
setTimeout('location.href = '/'', 10000);\
}\
</script>\
<form action='' method='get'>\
<table>\
<tr>\
<td>WiFi名称</td>\
<td><input class='n1' id='wifi_name' name='ap_ssid' type='text' value='%s'/></td>\
</tr>\
<tr>\
<td>WiFi密码</td>\
<td><input class='n1' id='wifi_password' name='ap_password' type='text' value='%s'/></td>\
</tr>\
<tr class='n5'>\
<td>蓝牙名称</td>\
<td><input class='n1' id='ble_name' name='ble_name' type='text' value='%s'/></td>\
</tr>\
<tr>\
<td></td>\
<td>\
<div style='float:left;'><input %s id='n3' id='is_connect_router'\
name='is_connect_router'\
type='checkbox'></div>\
<div style='float:left;'>是否连接路由器</div>\
</td>\
</tr>\
<tr>\
<td>路由器名称</td>\
<td><input class='n1' id='router_name' name='router_name' type='text' value='%s'/></td>\
</tr>\
<tr>\
<td>路由器密码</td>\
<td><input class='n1' id='router_password' name='router_password' type='text' value='%s'/></td>\
</tr>\
<tr>\
<td>内网IP</td>\
<td><input class='n1' id='inner_ip' name='inner_ip' type='text' value='%s'/></td>\
</tr>\
<tr class='spacer'></tr>\
<tr>\
<td>TCP Server 端口</td>\
<td><input class='n1' id='tcp_port' name='tcp_port' type='number' value='%d'/></td>\
</tr>\
<tr>\
<td>UDP Server 端口</td>\
<td><input class='n1' id='udp_port' name='udp_port' type='number' value='%d'/></td>\
</tr>\
<tr class='spacer'></tr>\
<tr>\
<td>串口波特率</td>\
<td>\
<select class='n2' class='default-action' id='bau' name='bau'>\
<option value='' selected='selected' hidden='hidden'>%s</option>\
<option value='1'>9600</option>\
<option value='2'>19200</option>\
<option value='3'>38400</option>\
<option value='4'>115200</option>\
<option value='5'>2000000</option>\
</select>\
</td>\
</tr>\
<tr>\
<td></td>\
<td>\
<div style='float:left;'><input class='save' id='save' type='submit' value='保存'/></div>\
</td>\
</tr>\
</table>\
</form>\
</div>\
</section>\
</body>\
</html>\
"
#endif //HTTP_SERVER1_PAGE_H