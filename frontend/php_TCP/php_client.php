<?php

// 采用$_REQUEST超全局数组来接收index.html页面post请求传递过来的数据
$buff=$_REQUEST["content"];

//tcp  client 
$serverIp="127.0.0.1";//服务端ip地址，如果你的客户端与服务端不在同一台电脑，请修改该ip地址
$serverPort=9006;//通信端口号
//设置超时时间
set_time_limit(0);
//创建套接字
 $sock= socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
if(!$sock) {
    echo "creat sock failed";
    exit();//创建套接字失败，结束程序
}
//发送数据到TCP的服务端
$connection = socket_connect($sock, $serverIp, $serverPort );
if($connection< 0) {
	echo "socket_connect failed!";
}
else {
	// echo "connection OK\n";
}

if(!socket_write($sock, $buff)) {
	echo "socket_write() failed: reason:".socket_strerror($sock)."\n";
	exit();
}

{
	// echo "发送成功，内容为:";
	// echo $buff;
}

$slen = socket_read($sock, 1024, PHP_NORMAL_READ);
// 接收json内容的长度，在传回来的消息头部
$len = intval($slen);

$buff = "";//清空缓冲区
// 真正读取json内容
socket_recv($sock, $buff, $len, MSG_WAITALL);

// 去掉接受到的字符串的首尾空格，返回给post请求的data
echo trim($buff)."\n";
// 关闭套接字
socket_close($sock);
?>

