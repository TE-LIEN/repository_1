function Tab2Clicked()
{
  document.getElementsByTagName("span")[0].style.color="black";
  document.getElementsByTagName("span")[1].style.color="black";
  document.getElementsByTagName("span")[2].style.color="blue";
  document.getElementsByTagName("span")[3].style.color="black";
  document.getElementsByTagName("span")[4].style.color="black";
  document.getElementsByTagName("span")[5].style.color="black";
  //clearTimeout(DTTimer);
	clearTimeout(RTValue);
	clearTimeout(DBGTimer);
	if(login_ok == 1)
	{	if(DtTimerStop)
		{	DtTimerStop = false;
			DTTimer=setTimeout("UpdateTime()", 1000);
		}
		Tab0.style.display='none';
		Tab1.style.display='none';
		Tab2.style.display='';
	  Tab3.style.display='none';
	  Tab4.style.display='none';
	  Tab5.style.display='none';
    SetSelectedValueByID("Port_Number", 0);
    UpdateDeviceDatapage();
	}
	else
	{
		Tab0.style.display='none';
		Tab1.style.display='none';
		Tab2.style.display='none';
		Tab3.style.display='none';
		Tab4.style.display='none';
		Tab5.style.display='none';
	}
}
function IntFloatNum(Data_Byte)
{
	byte_number.value = Data_Byte;
}

function GetAIMode()
{
	if (AI_Diff.checked)
		return AI_Diff.value;
	else if (AI_Single.checked)
		return AI_Single.value;
	else return "1";
}

function UpdatePortSettings(event)
{
	var sep = "|";
	var data = "1" + sep;
  var formula_id = Port_Number.selectedIndex;
  Port_Number.focus();
  data += strToInt(formula_id);
  if(makeRequest("GetDeviceParam.cgi", data) == -2)
  {	setTimeout("UpdatePortSettings()", 400);
  }
}
function UpdateDeviceDatapage()
{
	var sep = "|";
	var data = "1" + sep + "0";
  if(makeRequest("GetDeviceParam.cgi", data) == -2)
  {	setTimeout("UpdateDeviceDatapage()", 400);
  }
}
function SaveParam(skip=false)
{
	var sep = "|";
	var data = "2" + sep;
  data += GetSelectedValueByID("Port_Number") + sep;
  //console.log("save start:" + skip.toString());
	var command1, command2, command3, command4, command5, oldConsoleOn=0;
  command1 = Command_line_1.value;
	command2 = Command_line_2.value;
	command3 = Command_line_3.value;
	command4 = Command_line_4.value;
	command5 = Command_line_5.value;
  for(var i = 1; i <= 5; i++)
  {
		eval('if((command' + i + ').charCodeAt(0) == 35)	  command' + i + ' = command' + i + '.replace("#", "*")');
		eval('if((command' + i + ').charCodeAt(0) == 37)	  command' + i + ' = command' + i + '.replace("%", "/")');
		eval('if((command' + i + ').charCodeAt(0) == 126)	  command' + i + ' = command' + i + '.replace("~", "^")');
	}
	data +=GetSelectedValueByID("COM1_Protocol") + sep;
	data +=GetSelectedValueByID("COM2_Protocol") + sep;
	data +=GetSelectedValueByID("COM3_Protocol") + sep;
	/*if(GetSelectedValueByID("COM3_Protocol") == 30)
		oldConsoleOn = 1;*/
	data +=GetSelectedValueByID("ExtDevEth") + sep;
	data +=GetSelectedValueByID("ExtDevUsb") + sep;
  data += 1 + sep;
  for(i = 1; i <= 5; i++)
  {
		eval('if(Device_Enable_' + i + '.checked) {data += 1 + sep;} else {data += 0 + sep;}');
		eval('if(Device_Enable_' + i + '.checked) {data += GetSelectedValueByID("COM_Port_' + i + '") + sep;} else {data += 0 + sep;}');
		eval('if(Device_Enable_' + i + '.checked) {data += command' + i + '+ sep;} else {data += 0 + sep;}');
		eval('if(Device_Enable_' + i + '.checked) {data += Start_Byte_' + i + '.value + sep;} else {data += 0 + sep;}');
		//eval('data += 0 + sep;');
		eval('if(Device_Enable_' + i + '.checked) {data += GetSelectedValueByID("Data_type_' + i + '") + sep;} else {data += 0 + sep;}');
		eval('if(Device_Enable_' + i + '.checked) {data += GetSelectedValueByID("Data_seq_' + i + '") + sep;} else {data += 0 + sep;}');
		//eval('data += 0 + sep;')
	}
	data += GetCheckedValueById("ADC_power_Yes", "ADC_power_No", "1") + sep;
	data += GetAIMode() + sep;
	data += GetSelectedValueByID("senType0") + sep;
	data += GetSelectedValueByID("senType1") + sep;
	data += GetSelectedValueByID("senType2") + sep;
	data += GetSelectedValueByID("senType3") + sep;
	data += GetSelectedValueByID("senType4") + sep;
	
	//AR77 settings
	if((GetSelectedValueByID("COM1_Protocol") != 5) && (GetSelectedValueByID("COM2_Protocol") != 5))
	{	SetSelectedValueByID("ar77_MSEL", 0);
		ar77MaxDist.value = 0;
		ar77MinDist.value = 0;
		SetSelectedValueByID("ar77_x_range", 0);
		SetSelectedValueByID("ar77_resolution", 0);
		ar77Offset.value = 0;
		SetSelectedValueByID("wls_77g", 0);
	}
	data += GetSelectedValueByID("ar77_MSEL") + sep;
	if(ar77MaxDist.value.length >= 1)	
		data += ar77MaxDist.value + sep;
	else
		data += "0" + sep;
	if(ar77MinDist.value.length >= 1)	
		data += ar77MinDist.value + sep;
	else
		data += "0" + sep;
	data += GetSelectedValueByID("ar77_x_range") + sep;
	data += GetSelectedValueByID("ar77_resolution") + sep;
	data += ar77Offset.value + sep;
	data += GetSelectedValueByID("wls_77g") + sep;
	
	if((GetSelectedValueByID("COM1_Protocol") != 4) && (GetSelectedValueByID("COM2_Protocol") != 4))
	{	SetSelectedValueByID("avds_MSEL", 0);
		avdsStartAzimuth.value = 0;
		avdsEndAzimuth.value = 0;
		avdsStartRange.value = 0;
		avdsEndRange.value = 0;
		avdsDaulDist.value = 0;
		avdsTiltRadarAngle.value = 0;
		avdsTitleRangeBin.value = 0;
		avdsWaterLevel.value = 0;
		avdsMaxDist.value = 0;
	}
	data += GetSelectedValueByID("avds_MSEL") + sep;
	if(avdsStartAzimuth.value.length >= 1)	
		data += avdsStartAzimuth.value + sep;
	else
		data += "0" + sep;
	if(avdsEndAzimuth.value.length >= 1)	
		data += avdsEndAzimuth.value + sep;
	else
		data += "0" + sep;
	if(avdsStartRange.value.length >= 1)	
		data += avdsStartRange.value + sep;
	else
		data += "0" + sep;
	if(avdsEndRange.value.length >= 1)	
		data += avdsEndRange.value + sep;
	else
		data += "0" + sep;
	if(avdsDaulDist.value.length >= 1)	
		data += avdsDaulDist.value + sep;
	else
		data += "0" + sep;
	if(avdsTiltRadarAngle.value.length >= 1)	
		data += avdsTiltRadarAngle.value + sep;
	else
		data += "0" + sep;
	if(avdsTitleRangeBin.value.length >= 1)	
		data += avdsTitleRangeBin.value + sep;
	else
		data += "0" + sep;	
	if(avdsWaterLevel.value.length >= 1)	
		data += avdsWaterLevel.value + sep;
	else
		data += "0" + sep;
	if(avdsMaxDist.value.length >= 1)	
		data += avdsMaxDist.value + sep;
	else
		data += "0" + sep;

	data += osaDevNo.value + sep;
	data += osaSlaveId.value + sep;
	data += GetSelectedValueByID("osa24gAlert") + sep;
	data += osaAlertThreshold.value + sep;
	data += GetSelectedValueByID("osa24gWl") + sep;
	//data += GetSelectedValueByID("osa24gWlDi") + sep;
	data += osaMax.value + sep;
	data += osaMin.value + sep;
	data += GetSelectedValueByID("osa24gPwr") + sep;
	data += GetSelectedValueByID("osaPollInt") + sep;
	
	/*
	* AUDIO
	*/
	/*data += audioDevNo.value + sep;
	data += audioSlaveId.value + sep;
	data += GetSelectedValueByID("audioPwr") + sep;
	data += GetSelectedValueByID("audioWakeup") + sep;*/
	
	/*
	* IKW
	*/
	//data += smr01WlDevNo.value + sep;
	//data += smr01Altitude.value + sep;
	
	//data += smr61WlDevNo.value + sep;
	//data += smr61RadarDevNo.value + sep;
	//data += smr61IH.value + sep;
	//data += smr61Altitude.value + sep;
	
	data += compOsaDevNo.value + sep;
	data += GetSelectedValueByID("compOsaPwr") + sep;
	data += compWlsDevNo.value + sep;
	data += GetSelectedValueByID("compWlsPwr") + sep;
	data += compOsaAlertTrig.value + sep;
	data += compOsaAlertRet.value + sep;
	data += compOsaDist.value + sep;
	data += compOsaIh.value + sep;
	data += compOsaAlt.value + sep;
	
	data += GetSelectedValueByID("compLr110Ai") + sep;
	data += GetSelectedValueByID("compLR110Pwr") + sep;
	data += GetSelectedValueByID("compLr110WlsAiNo") + sep;
	data += GetSelectedValueByID("compLr110WlsPwr") + sep;
	data += compLr110AlertTrig.value + sep;
	data += compLr110AlertRet.value + sep;
	data += compLr110Dist.value + sep;
	data += compLr110Ih.value + sep;
	data += compLr110Alt.value + sep;
	data += compLr110Blind.value + sep;
	
	data += GetSelectedValueByID("rs485baudrate_sel") + sep;
	data += GetSelectedValueByID("rs485PDS_sel") + sep;
	data += GetSelectedValueByID("rs485_2baudrate_sel") + sep;
	data += GetSelectedValueByID("rs485_2PDS_sel") + sep;
	data += GetSelectedValueByID("rs232_1baudrate_sel") + sep;
  data += GetSelectedValueByID("rs232_1PDS_sel") + sep;
	//data += oldConsoleOn + sep;
	data += GetSelectedValueByID("mod_camera_interval") + sep;
	data += GetSelectedValueByID("cameraAlertInterval") + sep;
	data += GetSelectedValueByID("cameraResolution") + sep;
	data += GetSelectedValueByID("cameraQuality") + sep;
	
	data += vwDevNo.value + sep;
	data += vwSlaveId.value + sep;
	data += GetSelectedValueByID("vwPwr") + sep;
	data += vwPwrTime.value + sep;
	data += GetSelectedValueByID("vwPollInt") + sep;
	
	data += GetCheckedValueById("di_status_Yes", "di_status_No", "0") + sep;
	if(DI0_Wakeup.checked)  {data += 1 + sep;} else {data += 0 + sep;}
  if(DI1_Wakeup.checked)  {data += 1 + sep;} else {data += 0 + sep;}
  if(DI2_Wakeup.checked)  {data += 1 + sep;} else {data += 0 + sep;}
  if(DI3_Wakeup.checked)  {data += 1 + sep;} else {data += 0 + sep;}
	data += GetSelectedValueByID("dynamicRecSec") + sep;
	var confirmClick=true;
	if(!skip)
	{	if(!extend())
		{	setTimeout("SaveParam()", 200);
			//console.log("extend wait");
			return;
		}
		//return confirm('modify new settings? \n Please click \"Confirm\" to continue or \"Cancel\" to exit');
		//confirmClick = confirm('modify new settings? \n Please click \"Confirm\" to continue or \"Cancel\" to exit');
		confirmClick = settingAsk();
	}
	if(confirmClick)
  {	//var mkResult;
  	//console.log("setting confirm");
  	//mkResult = makeRequest("SaveParam.cgi", data);
  	//console.log("request result:" + mkResult);
  	//if(mkResult == -2)
  	if(makeRequest("SaveParam.cgi", data) == -2)
  	{	//console.log("wait for next save");
  		setTimeout("SaveParam(true)", 400);
  	}
  }
  else
  {	//console.log("cancel setting");
  	extendFinish();
  }
}
function setAiMode(adcMode)
{	senType0.disabled = false;
	senType1.disabled = false;
	senType2.disabled = false;
	senType3.disabled = false;
	if(adcMode == 0)	//differential
		senType4.disabled = true;
	else
		senType4.disabled = false;
}

function adcEnable(enable)
{	//var aiMode;
	if(enable == 1)
	{	setTableDisplay("compSiemensBoard", 1);
		AI_Diff.disabled = false;
		AI_Single.disabled = false;
		aiMode = GetAIMode();
		setAiMode(aiMode);
	}
	else
	{	setTableDisplay("compSiemensBoard", 0);
		if(compLr110Panel.style.maxHeight != null && compLr110Panel.style.maxHeight != 0)
			compSiemensBoard.click();
		AI_Diff.disabled = true;
		AI_Single.disabled = true;
		senType0.disabled = true;
		senType1.disabled = true;
		senType2.disabled = true;
		senType3.disabled = true;
		senType4.disabled = true;
	}
}

function isComModbus(comport)
{	var com1Prot = GetSelectedValueByID("COM1_Protocol");
	var com2Prot = GetSelectedValueByID("COM2_Protocol");
	var com3Prot = GetSelectedValueByID("COM3_Protocol");
	
	if(comport ==1)
	{	if(com1Prot == 1)
			return true;
	}
	else if(comport ==2)
	{	if(com2Prot == 1)
			return true;
	}
	else if(comport ==3)
	{	if(com3Prot == 1)
			return true;
	}
	else if(comport == 4)
	{	if(GetSelectedValueByID("ExtDevEth") == 1)
			return true;
	}
	return false;
}

function SelDisability(selectProtocol)
{	var i, j;
	var COMSelects   = document.getElementsByClassName('comSel');
	var arraySelects = document.getElementsByClassName('dataType');
	
	setComDisp();
	selectProtocol.focus();
	for(j = 0; j<COMSelects.length; j++)
	{	if(isComModbus(COMSelects[j].selectedIndex))
		{	for(i=0; i<11; i++)
				arraySelects[j].options[i].disabled = true;
			for(i=11; i<17; i++)
				arraySelects[j].options[i].disabled = false;
		}
		else
		{	for(i=0; i<11; i++)
				arraySelects[j].options[i].disabled = false;
			for(i=11; i<17; i++)
				arraySelects[j].options[i].disabled = true;
		}
	}
}

function toggleDisability(selectElement)
{	var i, j;
	var COMSelects = document.getElementsByClassName('comSel');
	var arraySelects = document.getElementsByClassName('dataType');
	
	selectElement.focus();
	for(j = 0; j<COMSelects.length; j++)
	{	if(isComModbus(selectElement.selectedIndex))
		{	for(i=0; i<11; i++)
				arraySelects[j].options[i].disabled = true;
			for(i=11; i<17; i++)
				arraySelects[j].options[i].disabled = false;
		}
		else
		{	for(i=0; i<11; i++)
				arraySelects[j].options[i].disabled = false;
			for(i=11; i<17; i++)
				arraySelects[j].options[i].disabled = true;
		}
	}
}
function byteorderDisability(byteorder_sel)
{	var i, j;
	var ArraySelects = document.getElementsByClassName('dataType');
	var SeqSelects   = document.getElementsByClassName('dataOrder');
	var OrderOption  = byteorder_sel.selectedIndex;

	byteorder_sel.focus();

	for(j = 0; j < ArraySelects.length; j++)
	{	if(byteorder_sel == ArraySelects[j])
		{	SeqSelects[j].options[0].disabled = false;
			switch(OrderOption)
			{	case 11:
				case 13:
				case 15:
							for(i=1; i<5; i++)
								SeqSelects[j].options[i].disabled = false;
							for(i=5; i<8; i++)
								SeqSelects[j].options[i].disabled = true;
							break;
				case 12:
				case 14:
							for(i=1; i<5; i++)
								SeqSelects[j].options[i].disabled = true;
							for(i=5; i<7; i++)
								SeqSelects[j].options[i].disabled = false;
							SeqSelects[j].options[7].disabled = true;
							break;
				case 16:
							for(i=1; i<7; i++)
								SeqSelects[j].options[i].disabled = true;
							SeqSelects[j].options[7].disabled = false;
							break;
			}
		}
	}
}

function avdsCalibration()
{	if(makeRequest("avdsCalibration.cgi", 0) == -2)
	{	setTimeout("avdsCalibration()", 400);
	}
}

function catchPic(mode)
{	btnGetCloudPic.disabled = true;
	btnCameraCatch.disabled = true;
	btnSendToCloud.disabled = true;
	
	if(mode == 0)
	{	imgMode = 0;
		imgGetGuid = 1;
		setTableDisplay("cameraSts", 0);
		getFwVer();
	}
	else if(mode == 1)
	{	imgMode = 1;
		setTableDisplay("cameraSts", 1);
		if(makeRequest("checkImg.cgi", "1") == -2)
		{	setTimeout("catchPic(1)", 400);
		}
	}
	else if(mode == 2)
	{	imgMode = 2;
		setTableDisplay("cameraSts", 1);
		if(makeRequest("checkImg.cgi", "0") == -2)
		{	setTimeout("catchPic(2)", 400);
		}
	}
	else if(mode == 3)
	{	if(makeRequest("checkImg.cgi", "2") == -2)
		{	setTimeout("catchPic(3)", 400);
		}
	}
	else if(mode == 4)
	{	if(makeRequest("checkImg.cgi", "3") == -2)
		{	setTimeout("catchPic(4)", 400);
		}
	}
}

function getOneImg()
{	var getLen;
	var data;
	var sep = "|";
	
	getLen = imgLen - imgPos;
	if(getLen > imgDlLen)
		getLen = imgDlLen;
	
	data = imgPos + sep;
	data += imgName + sep;
	data += getLen;
	
	if(makeRequest("sendGetImage.cgi", data) == -2)
	{	setTimeout("getOneImg()", 400);
	}
}


function devCmdClick(obj, idx, skip=false)
{	var comName = 'COM_Port_' + idx;
	var comEn = 'Device_Enable_' + idx
	//var extDevObj = document.getElementsByClassName(comName);
	var val = GetSelectedValueByID(comName);
	var dat;
	var enObj = document.getElementById(comEn);
	if(enObj.checked)	
	if(val == 4)
	{	console.log("is Eth port");
		if(!skip)
		{	if(!extend())
			{	console.log("re-try extend");
				setTimeout("devCmdClick(obj, idx)", 50);
				return;
			}
		}
		var result = prompt("modBus Tcp IP:");
		dat = result + ",";
		var result = prompt("modBus Tcp Command:");
		dat += result;
		extendFinish();
		obj.value = dat;
	}	
}

function selOsa24gAlert()
{	var sel = GetSelectedValueByID("osa24gAlert");
	if(sel == "0")
	{	setTableDisplay("osaThresholdVal", 0);
	}
	else
	{	setTableDisplay("osaThresholdVal", 1);
	}
}

function selOsaWls()
{	var sel = GetSelectedValueByID("osa24gWl");
	if(sel != "0")
	{	SetSelectedValueByID("osa24gAlert", "0");
	}
	selOsa24gAlert();
}