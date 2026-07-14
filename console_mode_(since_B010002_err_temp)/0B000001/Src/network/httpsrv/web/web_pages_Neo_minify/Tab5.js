function Tab5Clicked()
{document.getElementsByTagName("span")[0].style.color="black";document.getElementsByTagName("span")[1].style.color="black";document.getElementsByTagName("span")[2].style.color="black";document.getElementsByTagName("span")[3].style.color="black";document.getElementsByTagName("span")[4].style.color="black";document.getElementsByTagName("span")[5].style.color="blue";clearTimeout(RTValue);if(login_ok==1)
{Tab0.style.display='none';Tab1.style.display='none';Tab2.style.display='none';Tab3.style.display='none';Tab4.style.display='none';Tab5.style.display='';getFwVer();}
else
{Tab0.style.display='none';Tab1.style.display='none';Tab2.style.display='none';Tab3.style.display='none';Tab4.style.display='none';Tab5.style.display='none';}}
function getFwVer()
{if(makeRequest("GetFWVersion.cgi",0)==-2)
{setTimeout("getFwVer()",400);}}
function fwUpdate()
{if(makeRequest("SetBootParam.cgi",0)==-2)
{setTimeout("fwUpdate()",400);}}
function setBootParam()
{var password=prompt("Please enter the password!","")
if(password==("FWUPDATE"))
{fwUpdate();}
else
{if(password=="")
{alert('Empty! Please enter the correct password.');}
else if(password==null){}
else if(password!=("FWUPDATE"))
{alert('Error! Please enter the correct password.');}}}
function DebugMessage()
{if(makeRequest("GetDebug.cgi",0)==-2)
{setTimeout("DebugMessage()",400);}
else
{DBGTimer=setTimeout("DebugMessage()",1000);clearTimeout(DTTimer);DtTimerStop=true;}}