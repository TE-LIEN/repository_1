function makeRequest(url,data)
{var http_request=false;if(unauthorized)
return-2
if(data_received)
return-2
url=url+"?sid="+window.sessionStorage.getItem("S")+"&T="+window.sessionStorage.getItem("T")+"&D="+data;is_lan_alive=0;data_received=1;if(window.XMLHttpRequest)
{http_request=new XMLHttpRequest();if(http_request.overrideMimeType){http_request.overrideMimeType('text/xml');}}
else if(window.ActiveXObject)
{try{http_request=new ActiveXObject("Msxml2.XMLHTTP4.0");}
catch(e)
{try{http_request=new ActiveXObject("Microsoft.XMLHTTP");}
catch(e){}}}
if(!http_request){alert('Giving up : Cannot create an XMLHTTP instance');data_received=0;return-1;}
http_request.onreadystatechange=function(){alertContents(http_request);};http_request.open('GET',url,true);http_request.send(null);rspTimer=setTimeout("rspTimeout()",4000);return 0;}
function parserResp1(rsp)
{let authErr=rsp.indexOf("Unauthorized");let tPos=rsp.indexOf("&T=");let tPosE=rsp.indexOf("\n");let tt=rsp.substring(tPos+3,tPosE);var parsed=rsp.substring(tPosE+1,rsp.length);if((authErr!=-1)&&(authErr<50))
{if(unauthorized)
return
unauthorized=true
DtTimerStop=true;clearTimeout(DTTimer);clearTimeout(rspTimer);alert("Unauthorized or timeout! \n Re-login");setTimeout("location.reload()",1000);return;}
window.sessionStorage.setItem("T",tt);clearTimeout(rspTimer);parse_resp(parsed);}
function alertContents(http_request)
{if(http_request.readyState==4)
{is_lan_alive=1;if(http_request.status==200)
{parserResp1(http_request.responseText);}
else
{data_received=0;}}}
function GetCheckedValueById(yesObjId,noObjId,defaultVal)
{var yesObj=document.getElementById(yesObjId);var noObj=document.getElementById(noObjId);if(yesObj.checked)
return yesObj.value;else if(noObj.checked)
return noObj.value;else
return defaultVal;}
function GetSelectedValueByID(selObjID)
{var obj=document.getElementById(selObjID);var idx=obj.selectedIndex;return obj.options[idx].value;}
function SetSelectedValueByID(selObjID,index)
{var obj=document.getElementById(selObjID);obj.options[index].selected=true;}
function setSelValById(objId,valToSel)
{var obj=document.getElementById(objId);obj.value=valToSel;}
function ClearTextIfNone(selObj,inpObj)
{var sObj=document.getElementById(selObj);var iObj=document.getElementById(inpObj);if(sObj.selectedIndex==0)iObj.value="";}
function Trim(str)
{return str.replace(/^\s\s*/,'').replace(/\s\s*$/,'');}
function isInt(value)
{var er=/^[0-9]+$/;return(er.test(value))?true:false;}
function settingAsk()
{return confirm('modify new settings? \n Please click \"Confirm\" to continue or \"Cancel\" to exit');}
function extend()
{if(makeRequest("Extend.cgi",0)==-2)
{return false;}
return true;}
function extendFinish()
{if(makeRequest("UnExtend.cgi",0)==-2)
{setTimeout("extendFinish()",400);}}
function rspTimeout()
{data_received=0;}
function setTableDisplay(obj,display)
{var trObj=document.getElementById(obj);if(trObj)
{if(display)
{try{trObj.style.display="table-row";}
catch(e)
{trObj.style.display="block";}}
else
{trObj.style.display='none';}}}
function setChkedById(objId)
{var obj=document.getElementById(objId);obj.checked=true;}
function padZero(num,size)
{num=num.toString();while(num.length<size)num="0"+num;return num;}