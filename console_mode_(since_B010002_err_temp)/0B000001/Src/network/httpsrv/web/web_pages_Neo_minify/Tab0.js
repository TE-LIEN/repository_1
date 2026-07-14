function Tab0Clicked()
{if(initDone==0)
{initDone=1;initial();}
clearTimeout(RTValue);clearTimeout(DBGTimer);Tab0.style.display='';Tab1.style.display='none';Tab2.style.display='none';Tab3.style.display='none';Tab4.style.display='none';Tab5.style.display='none';document.getElementsByTagName("span")[0].style.color="blue";document.getElementsByTagName("span")[1].style.color="black";document.getElementsByTagName("span")[2].style.color="black";document.getElementsByTagName("span")[3].style.color="black";document.getElementsByTagName("span")[4].style.color="black";document.getElementsByTagName("span")[5].style.color="black";document.form1.autorun.value="";document.form1.power.value="";document.form1.new_username.value="";document.form1.new_password.value="";document.form1.confirm_password.value="";if(login_ok==1)
{if(DtTimerStop)
{DtTimerStop=false;DTTimer=setTimeout("UpdateTime()",1000);}}}
function check()
{var sep="|";var data="2"+sep;var autorun=document.form1.autorun.value;var power=document.form1.power.value;data+=window.btoa(autorun)+sep;data+=window.btoa(power);let token=Math.floor((Math.random()*10000)+1);window.sessionStorage.setItem("S",token);makeRequest("Login.cgi",data);}
function inputKeyUp(e)
{e.which=e.which||e.keyCode;if(e.which==13)check();}
function reset_password()
{if(!isValidPassword())return;if(login_ok==1&&(document.form1.new_password.value==document.form1.confirm_password.value))
{var sep="|";var data="8"+sep;var username=document.form1.new_username.value+"ly";var pw=document.form1.new_password.value+"02";data+=window.btoa(username)+sep;data+=window.btoa(pw);if(makeRequest("SavePassword.cgi",data)==-2)
{setTimeout("reset_password()",400);}}
else if(login_ok==0)
{document.form1.autorun.value="";document.form1.power.value="";document.form1.new_username.value="";document.form1.new_password.value="";document.form1.confirm_password.value="";alert("Please sign in first!");}
else if(document.form1.new_password.value!=document.form1.confirm_password.value)
{document.form1.autorun.value="";document.form1.power.value="";document.form1.new_username.value="";document.form1.new_password.value="";document.form1.confirm_password.value="";alert("Please check your password!");}}
function setAccordion()
{var acc=document.getElementsByClassName("accordion");var i;for(i=0;i<acc.length;i++)
{acc[i].addEventListener("click",function(){this.classList.toggle("active");var panel=this.nextElementSibling;if(panel.style.maxHeight)
panel.style.maxHeight=null;else
panel.style.maxHeight=panel.scrollHeight+"px";});}}
function setBaudOpt()
{var baudArr=[1200,4800,9600,19200,38400,57600,115200];var baudObj=document.getElementsByClassName("baudrateSel");var i,j,opt;for(i=0;i<baudObj.length;i++)
{for(j=0;j<baudArr.length;j++)
{opt=document.createElement("option");opt.value=baudArr[j].toString();opt.innerHTML=baudArr[j];baudObj[i].add(opt);}}}
function setFmtOpt()
{var fmtArr=["N-8-1","N-8-2","E-8-1","O-8-1"];var fmtObj=document.getElementsByClassName("fmtSel");var i,j,opt;for(i=0;i<fmtObj.length;i++)
{for(j=0;j<fmtArr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=fmtArr[j];fmtObj[i].add(opt);}}}
function setComdSelOpt()
{var arr=["NONE","COM1","COM2","COM3","ETH","HS-USB"];var comSelObj=document.getElementsByClassName("comSel");var i,j,opt;for(i=0;i<comSelObj.length;i++)
{for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];comSelObj[i].add(opt);}}}
function setDataTypeOpt()
{var arr=["00","01","02","03","04","05","06","07","08","09","10","Float","Int16","Int32","U_Int16","U_Int32","BOOL"];var dataTypeObj=document.getElementsByClassName("dataType");var i,j,opt;for(i=0;i<dataTypeObj.length;i++)
{for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];dataTypeObj[i].add(opt);}}}
function setDataOrderOpt()
{var arr=["NONE","1. (L)L (L)H (H)L (H)H","2. (L)H (L)L (H)H (H)L","3. (H)L (H)H (L)L (L)H","4. (H)H (H)L (L)H (L)L","5. H L","6. L H","7. BOOL"];var dataOrderObj=document.getElementsByClassName("dataOrder");var i,j,opt;for(i=0;i<dataOrderObj.length;i++)
{for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];dataOrderObj[i].add(opt);}}}
function onkeyupReplace(obj,replaceType)
{if(replaceType==0)
obj.value=obj.value.replace(/[^0-9]/g,'');else if(replaceType==1)
obj.value=obj.value.replace(/[^_-a-zA-Z0-9~!@$%^&*/()+=?]/g,'');else if(replaceType==2)
obj.value=obj.value.replace(/[^.0-9]/g,'');else if(replaceType==3)
obj.value=obj.value.replace(/[^-.@?*#a-zA-Z0-9]/g,'');else if(replaceType==4)
obj.value=obj.value.replace(/[^-.@?*/#a-zA-Z0-9]/g,'');else if(replaceType==5)
obj.value=obj.value.replace(/[^-.0-9]/g,'');else if(replaceType==6)
obj.value=obj.value.replace(/[^a-fA-F0-9]/g,'');}
function setKeyup()
{var objs=document.getElementsByClassName("keyup0");var i;for(i=0;i<objs.length;i++)
objs[i].onkeyup=function(){onkeyupReplace(this,0);};objs=document.getElementsByClassName("keyup1");for(i=0;i<objs.length;i++)
objs[i].onkeyup=function(){onkeyupReplace(this,1);};objs=document.getElementsByClassName("keyup2");for(i=0;i<objs.length;i++)
objs[i].onkeyup=function(){onkeyupReplace(this,2);};objs=document.getElementsByClassName("keyup3");for(i=0;i<objs.length;i++)
objs[i].onkeyup=function(){onkeyupReplace(this,3);};objs=document.getElementsByClassName("keyup4");for(i=0;i<objs.length;i++)
objs[i].onkeyup=function(){onkeyupReplace(this,4);};objs=document.getElementsByClassName("keyup5");for(i=0;i<objs.length;i++)
objs[i].onkeyup=function(){onkeyupReplace(this,5);};objs=document.getElementsByClassName("keyup6");for(i=0;i<objs.length;i++)
objs[i].onkeyup=function(){onkeyupReplace(this,6);};}
function setAiOpt()
{var arr=["Current(mA)","Voltage(V)"];var aiObj=document.getElementsByClassName("aiSel");var i,j,opt;for(i=0;i<aiObj.length;i++)
{for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];aiObj[i].add(opt);}}}
function setDoAutoOpt()
{var arr=["NONE","A","B"];var doObj=document.getElementsByClassName("doAutoSel");var i,j,opt;for(i=0;i<doObj.length;i++)
{for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];doObj[i].add(opt);}}}
function setDiOpt()
{var arr=["Engine Failure","Oil Level","Out of Water","Engine Start"];var diObj=document.getElementsByClassName("diSel");var i,j,k,opt;for(i=0;i<diObj.length;i++)
{if(i>1)
k=1;else
k=0;for(j=0;j<arr.length-k;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];diObj[i].add(opt);}}}
function setDoMapOpt()
{var arr=["No Wired","DO-0(M2 PIN7)","DO-1(M2 PIN8)"];var doObj=document.getElementsByClassName("doMapSel");var i,j,k,opt;for(i=0;i<doObj.length;i++)
{doObj[i].innetHEML="";for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];doObj[i].add(opt);}}}
function setAiSensorType1()
{var arr=["4-20mA","0-20mA","\u00B120mA"];var doObj=document.getElementsByClassName("aiSensorType1");var i,j,k,opt;for(i=0;i<doObj.length;i++)
{doObj[i].innetHEML="";for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];doObj[i].add(opt);}}}
function setAiSensorType2()
{var arr=["+500mV","+5V","+10V","\u00B16V"];var doObj=document.getElementsByClassName("aiSensorType2");var i,j,k,opt;for(i=0;i<doObj.length;i++)
{doObj[i].innetHEML="";for(j=0;j<arr.length;j++)
{opt=document.createElement("option");opt.value=j.toString();opt.innerHTML=arr[j];doObj[i].add(opt);}}}
function initial()
{setAccordion();setBaudOpt();setFmtOpt();setComdSelOpt();setDataTypeOpt();setDataOrderOpt();setAiOpt();setDiOpt();setDoAutoOpt();setKeyup();setDoMapOpt();setAiSensorType1();setAiSensorType2();}