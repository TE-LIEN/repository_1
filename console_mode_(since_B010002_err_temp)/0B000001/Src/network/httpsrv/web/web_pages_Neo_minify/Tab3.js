function Tab3Clicked()
{document.getElementsByTagName("span")[0].style.color="black";document.getElementsByTagName("span")[1].style.color="black";document.getElementsByTagName("span")[2].style.color="black";document.getElementsByTagName("span")[3].style.color="blue";document.getElementsByTagName("span")[4].style.color="black";document.getElementsByTagName("span")[5].style.color="black";clearTimeout(RTValue);clearTimeout(DBGTimer);if(login_ok==1)
{if(DtTimerStop)
{DtTimerStop=false;DTTimer=setTimeout("UpdateTime()",1000);}
Tab0.style.display='none';Tab1.style.display='none';Tab2.style.display='none';Tab3.style.display='';Tab4.style.display='none';Tab5.style.display='none';SetSelectedValueByID("AI_Number",0);if(isSu==2)
{setTableDisplay("doMapBoard",1);}
UpdateNodeDatapage();}
else
{setTableDisplay("doMapBoard",0);if(doMapPanel.style.maxHeight!=null&&doMapPanel.style.maxHeight!=0)
doMapBoard.click();Tab0.style.display='none';Tab1.style.display='none';Tab2.style.display='none';Tab3.style.display='none';Tab4.style.display='none';Tab5.style.display='none';}}
function UpdateNodeData(coef)
{var sep="|";var data="1"+sep;var formula_id=coef.selectedIndex;coef.focus();data+=strToInt(formula_id);if(makeRequest("GetNodeParam.cgi",data)==-2)
{setTimeout("UpdateNodeData(coef)",400);}}
function GetNodeParam1()
{var sep="|";var data="5"+sep;if(makeRequest("GetNodeParam.cgi",data)==-2)
{setTimeout("GetNodeParam1()",400);}}
function UpdateNodeDatapage()
{var sep="|";var data="1"+sep+"0";if(makeRequest("GetNodeParam.cgi",data)==-2)
{setTimeout("UpdateNodeDatapage()",400);}}
function SaveNodeParam(skip=false)
{var sep="|";var data="3"+sep;data+=GetSelectedValueByID("AI_Number")+sep;for(var i=0;i<=7;i++)
{var enable=document.getElementById("Channel_Enable_"+i);var alias=document.getElementById("Alias_"+i);var srv2Alias=document.getElementById("srv2Alias_"+i);if(enable.checked)
{eval('data += 1 + sep;');eval('data += Device_no_'+i+'.value + sep;');eval('data += Item_num_'+i+'.value + sep;');eval('data += EQN_'+i+'.value + sep;');eval('data += Filter_Enable_'+i+'.value + sep;');if(alias.value!="")
data+=alias.value+sep;else
data+=0+sep;if(srv2Alias.value!="")
data+=srv2Alias.value+sep;else
data+=0+sep;}
else
{data+=0+sep+0+sep+0+sep+0+sep+0+sep+0+sep+0+sep;}}
var confirmClick=true;if(!skip)
{if(!extend())
{setTimeout("SaveNodeParam()",200);return;}
confirmClick=settingAsk();}
if(confirmClick)
{if(makeRequest("SaveNodeParam.cgi",data)==-2)
{setTimeout("SaveNodeParam(true)",400);}}
else
{extendFinish();}}
function SaveDoParam(skip=false)
{var sep="|";var blank="!";var data="4"+sep;if(DO_0_Output.checked){data+=1+sep;}else{data+=0+sep;}
if(DO_1_Output.checked){data+=1+sep;}else{data+=0+sep;}
if(DO_5V_Output.checked){data+=1+sep;}else{data+=0+sep;}
data+=GetSelectedValueByID("DO_Master_Mode_0");var confirmClick=true;if(!skip)
{if(!extend())
{setTimeout("SaveDoParam()",200);return;}
confirmClick=settingAsk();}
if(confirmClick)
{if(makeRequest("SaveDOParam.cgi",data)==-2)
{setTimeout("SaveDoParam(true)",400);}}
else
{extendFinish();}}
function CoefChange(coef)
{var sep="|";var data="1"+sep;var formula_id=coef.selectedIndex;coef.focus();data+=strToInt(formula_id);if(makeRequest("GetCoef.cgi",data)==-2)
{setTimeout("CoefChange(coef)",400);}}
function CoefChange_1(coef)
{var sep="|";var data="3"+sep;var formula_id=coef.selectedIndex;coef.focus();data+=strToInt(formula_id+10);if(makeRequest("GetCoef.cgi",data)==-2)
{setTimeout("CoefChange_1(coef)",400);}}
function SaveFormulaParam(skip=false)
{var sep="|";var blank="!";var data="7"+sep;data+=GetSelectedValueByID("Coef_Number")+sep;data+=Coef_A.value+blank;data+=Coef_B.value+blank;data+=Coef_C.value;var confirmClick=true;if(!skip)
{if(!extend())
{setTimeout("SaveFormulaParam()",200);return;}
confirmClick=settingAsk();}
if(confirmClick)
{if(makeRequest("SaveFormulaParam.cgi",data)==-2)
{setTimeout("SaveFormulaParam(true)",400);}}
else
{extendFinish();}}
function SaveFormulaParam_1(skip=false)
{var sep="|";var blank="!";var data="9"+sep;data+=GetSelectedValueByID("Coef_Number_1")+sep;data+=Coef_A_1.value+blank;data+=Coef_B_1.value;var confirmClick=true;if(!skip)
{if(!extend())
{setTimeout("SaveFormulaParam_1()",200);return;}
confirmClick=settingAsk();}
if(confirmClick)
{if(makeRequest("SaveFormulaParam.cgi",data)==-2)
{setTimeout("SaveFormulaParam_1(true)",400);}}
else
{extendFinish();}}
function doMapChg(idx)
{var selects=document.getElementsByClassName("doMapSel");var used1=-1;var used2=-1;for(var i=0;i<selects.length;i++)
{if(selects[i].selectedIndex===1)
{used1=i;}
if(selects[i].selectedIndex===2)
{used2=i;}}
for(var i=0;i<selects.length;i++)
{var sel=selects[i];for(var j=0;j<sel.options.length;j++)
{sel.options[j].disabled=false;}
if(used1!==-1&&used1!==i)
sel.options[1].disabled=true;if(used2!==-1&&used2!==i)
sel.options[2].disabled=true;}}
function saveDoMap(skip=false)
{var sep="|";var blank="!";var data="10"+sep;data+=GetSelectedValueByID("doMapSel0")+sep;data+=GetSelectedValueByID("doMapSel1")+sep;data+=GetSelectedValueByID("doMapSel2")+sep;data+=GetSelectedValueByID("doMapSel3")+sep;var confirmClick=true;if(!skip)
{if(!extend())
{setTimeout("SaveDOMap()",200);return;}
confirmClick=settingAsk();}
if(confirmClick)
{if(makeRequest("SaveDOMap.cgi",data)==-2)
{setTimeout("saveDoMap(true)",400);}}
else
{extendFinish();}}