<?php
if($_GET['userid'])  $userlable=$_GET['userid'];
else {$userlable = "anonymous";}
if($_GET['playerport']) $playerport=$_GET['playerport'];
else {$playerport = "1";}
if($_GET['httpport'])  $httpport=$_GET['httpport'];
else {$httpport = "2";}
if($_GET['conffilename']) $configfilename=$_GET['conffilename'];
else {$configfilename = "multiLaser.test3.cfg";}
if($_GET['serverport'])  $svport=$_GET['serverport'];
else {$svport = "5000";}
if($_GET['map'])  $mapname=$_GET['map'];
else {$mapname = "cave_compact.png";}

$envpath = "./";
$configfilepath = $envpath;
$configfilepath .=$userlable."/".$configfilename;
$command = "player";
$commandhead = "playerRun";
$otherPara = "-g";
$finalcommand = $commandhead;
$finalcommand .=" ".$command." ".$configfilepath." ".$otherPara." -p ".$playerport." -h ".$httpport."\n";
echo $finalcommand;
$mappath = $envpath;
$mappath .=$userlable."/".$mapname ;

/**************************************************
connect to server and send command using socket
**************************************************/
// 建立客户端的socet连接 

$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);  
$connection = socket_connect($socket, '127.0.0.1', $svport);    //连接服务器端socket  
if (!socket_write($socket, "$finalcommand")) {
            echo "send command failed\n";  
}
else{
	echo "send success\n";
} 
$buf = 'This is my buffer.'; 
if (false !== ($bytes = socket_recv($socket, $buf, 2048, 0))) {
	//echo "$buf\n";
$token = strtok($buf," ");
if($token !== false) $playerport=$token;
$token = strtok(" "); 
if($token !== false) $httpport=$token;
echo ":".$playerport.":".$httpport.":";
} else {
    echo "socket_recv() failed; reason: " . socket_strerror(socket_last_error($socket)) . "\n";
}  
socket_close ($socket ) ;

/***********************************************
jump to simulation url + send some data
************************************************/

header("Location: http://localhost/car/webstage.php?httpport=$httpport&map=$mappath");
?> 
