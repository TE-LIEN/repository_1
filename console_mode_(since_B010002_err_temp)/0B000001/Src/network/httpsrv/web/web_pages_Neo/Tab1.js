function Tab1Clicked()
{
	clearTimeout(DTTimer);
	clearTimeout(RTValue);
	clearTimeout(DBGTimer);
  document.getElementsByTagName("span")[0].style.color="black";
  document.getElementsByTagName("span")[1].style.color="blue";
  document.getElementsByTagName("span")[2].style.color="black";
  document.getElementsByTagName("span")[3].style.color="black";
  document.getElementsByTagName("span")[4].style.color="black";
  document.getElementsByTagName("span")[5].style.color="black";
	if(login_ok == 1)
	{
		Tab0.style.display='none';
		Tab1.style.display='';
		Tab2.style.display='none';
		Tab3.style.display='none';
		Tab4.style.display='none';
		Tab5.style.display='none';
		GetCurrParam();
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
function isValidPassword()
{
	if (document.form1.new_username.value.length >= 1 && document.form1.new_username.value.length <= 30)
	{
		return true;
	}	else {
	  alert("Username length is wrong!");
	  return false;
	}
	if (document.form1.new_password.value.length >= 1 && document.form1.new_password.value.length <= 30)
	{
		return true;
	}	else {
		alert("Password length is wrong!");
	  return false;
	}
}
function isValidIPaddr(ipaddr)
{
	var re = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/;
	if (re.test(ipaddr))
	{
	  var parts = ipaddr.split(".");
	  if (parseInt(parseFloat(parts[0])) == 0)     { return false; }
	  for (var i=0; i<parts.length; i++) {
	     if (parseInt(parseFloat(parts[i])) > 255) { return false; }
	  }
	  return true;
	} else {
	  return false;
	}
}
function validateIPaddr()
{
	var ip = mod_ip_addr.value;
	if (!isValidIPaddr(ip)) { alert("Invalid IP Address: " + ip); return false; }
	ip = mod_ip_mask.value;
	if (!isValidIPaddr(ip)) { alert("Invalid IP Mask: " + ip);    return false; }
	ip = mod_ip_gateway.value;
	if (!isValidIPaddr(ip)) { alert("Invalid IP Gateway: " + ip); return false; }
	ip = modIpDns.value;
	if (!isValidIPaddr(ip)) { alert("Invalid IP DNS: " + ip); return false; }
	return true;
}
function checkbatdiff()
{
	if (parseFloat(mod_bat_on_diff.value) < 0.5)
	{
		alert("Bat. LVD reboot difference value should be bigger than 0.5.");
		return false;
	}
	if (parseFloat(mod_bat_on_diff.value) + parseFloat(mod_bat_off.value) > 15)
	{
		alert("Bat. reboot value should be less than 14!");
		return false;
	}
	return true;
}
function GetCheckedalerton()
{
	if (alerton.checked)
		return alerton.value;
	else
    return "0";
}

function IntToIPstr(int)
{
	var part1 = int & 255;
	var part2 = ((int >> 8) & 255);
	var part3 = ((int >> 16) & 255);
	var part4 = ((int >> 24) & 255);
	return part4 + "." + part3 + "." + part2 + "." + part1;
}
function IPstrToInt(ipstr)
{
	var parts = ipstr.split(".");
	var res = 0;
	res += parseInt(parts[0], 10) << 24;
	res += parseInt(parts[1], 10) << 16;
	res += parseInt(parts[2], 10) << 8;
	res += parseInt(parts[3], 10);
	return res;
}
function strToInt(datastr)
{
	var res = 0;
	res = parseInt(datastr, 10);
	return res;
}
function address_point(add1, add2, point1, point2)
{
	var res = 0;
	res += parseInt(add1, 16) << 24;
	res += parseInt(add2, 16) << 16;
	res += parseInt(point1, 10) << 8;
	res += parseInt(point2, 10);
	return res;
}
function addZero(i)
{
	if (i < 10) i = "0" + i;
	return i;
}
function GetDTString()
{
	var d = new Date();
	var data = "";
	var month = d.getMonth() + 1;

	data += d.getFullYear()          + ".";
	data += addZero(month)           + ".";
	data += addZero(d.getDate())     + " ";
	data += addZero(d.getHours())    + ":";
	data += addZero(d.getMinutes())  + ":";
	data += addZero(d.getSeconds());
	return data;
}
function SaveModifiedParam(skip=false)
{	var mdvpnStr;
	var mdvpnType;
  if (!checkbatdiff())        return;
	if (!validateIPaddr())      return;
	var sep = "|";
	var data = "1" + sep;
	var reqSts;
	data += GetSelectedValueByID("mod_rec_sec") + sep;
	//data += GetSelectedValueByID("mod_camera_interval") + sep;
	//data += GetSelectedValueByID("cameraAlertInterval") + sep;
	//data += GetSelectedValueByID("cameraResolution") + sep;
	//data += GetSelectedValueByID("cameraQuality") + sep;
	data += GetSelectedValueByID("mod_autosend_sec") + sep;
  if(mod_ip_addr.value == "" || mod_ip_addr.value == null)  mod_ip_addr.value = "192.168.255.1";
  mdvpnType = GetSelectedValueByID("ethMdvpnType");
  mdvpnStr = "";
  if(mdvpnType == 1)
  	mdvpnStr = "[MDVPN]";
  else if(mdvpnType == 2)
  	mdvpnStr = "[mdvpn]";
  mdvpnStr += IPstrToInt(mod_ip_addr.value);
	data += mdvpnStr + sep;
  if(mod_ip_mask.value == "" || mod_ip_mask.value == null)  mod_ip_mask.value = "255.255.255.0";
	data += IPstrToInt(mod_ip_mask.value) + sep;
  if(mod_ip_gateway.value == "" || mod_ip_gateway.value == null)  mod_ip_gateway.value = "192.168.255.254";
	data += IPstrToInt(mod_ip_gateway.value) + sep;
	data += GetSelectedValueByID("pwrSaving") + sep;
	data += GetSelectedValueByID("mod_Weak_Up_Interval") + sep;
  if(mod_Senslink_APN.value == "" || mod_Senslink_APN.value == null)  mod_Senslink_APN.value = "internet";
  mdvpnType = GetSelectedValueByID("lteMdvpnType");
  mdvpnStr = "";
	if(mdvpnType==1)
		mdvpnStr = "[MDVPN]";
	else if(mdvpnType==2)
		mdvpnStr = "[mdvpn]"
	mdvpnStr += mod_Senslink_APN.value;
	data += mdvpnStr + sep;
  if(mod_Senslink_Address.value == "" || mod_Senslink_Address.value == null)  mod_Senslink_Address.value = "0";
	data += mod_Senslink_Address.value + sep;
  if(mod_Senslink_Port.value == "" || mod_Senslink_Port.value == null)  mod_Senslink_Port.value = "0";
	data += mod_Senslink_Port.value + sep;
  if(mod_Senslink_Address2.value == "" || mod_Senslink_Address2.value == null)  mod_Senslink_Address2.value = "0";
	data += mod_Senslink_Address2.value + sep;
  if(mod_Senslink_Port2.value == "" || mod_Senslink_Port2.value == null)  mod_Senslink_Port2.value = "0";
	data += mod_Senslink_Port2.value + sep;
  data += modIpDns.value + sep;
  //data += GetCheckedValueById("di_status_Yes", "di_status_No", "0") + sep;
  if(mod_bat_off.value == "" || mod_bat_off.value == null)  mod_bat_off.value = "11.5";
	data += mod_bat_off.value + sep;
  if(mod_bat_on_diff.value == "" || mod_bat_on_diff.value == null)  mod_bat_on_diff.value = "0.5";
	data += mod_bat_on_diff.value + sep;
	data += GetCheckedValueById("Mode_LAN", "Mode_MOB", "0") + sep;
	data += GetCheckedValueById("Water_level_Yes", "Water_level_No", "1") + sep;
	data += GetCheckedValueById("GATE_YES", "GATE_No", "1") + sep;
	data += GetCheckedValueById("PUMP_YES", "PUMP_No", "1") + sep;
	data += GetCheckedValueById("sewerImEn", "sewerImDis", "1") + sep;
	data += GetCheckedValueById("autoFotaEn", "autoFotaDis", "1") + sep;
	data += GetSelectedValueByID("imageServer") + sep;
	data += GetSelectedValueByID("mod_Mobile_interval") + sep;
  data += GetSelectedValueByID("mod_comm_module") + sep;
  data += GetSelectedValueByID("mod_comm_module1") + sep;
  //if(DI0_Wakeup.checked)  {data += 1 + sep;} else {data += 0 + sep;}
  //if(DI1_Wakeup.checked)  {data += 1 + sep;} else {data += 0 + sep;}
  //data += GetSelectedValueByID("mod_sleep_times") + sep;
  //data +=	"115200" + sep;
  data +=	GetCheckedalerton() + sep;
  if(alerton_level.value == "" || alerton_level.value == null)  alerton_level.value = "0";
  data +=	alerton_level.value + sep;
  if(mod_Senslink_APN_NB.value == "" || mod_Senslink_APN_NB.value == null)  mod_Senslink_APN_NB.value = "internet";
  data +=	mod_Senslink_APN_NB.value + sep;
  if(mod_Senslink_PLMN.value == "" || mod_Senslink_PLMN.value == null)  mod_Senslink_PLMN.value = "46692";
  data +=	mod_Senslink_PLMN.value + sep;
  data += GetSelectedValueByID("mod_SIMAuthentication") + sep;
  if(mod_SIMAccount.value == "" || mod_SIMAccount.value == null)  mod_SIMAccount.value = "0";
  data += window.btoa(mod_SIMAccount.value) + sep;                              // base64 encoder
  if(mod_SIMPassword.value == "" || mod_SIMPassword.value == null)  mod_SIMPassword.value = "0";
  data += window.btoa(mod_SIMPassword.value) + sep;                             // base64 encoder
  //data += GetCheckedHistoryValue() + sep;
  data += GetCheckedValueById("History_record_Yes", "History_record_No", "0") + sep;
  data += wisunNodeCount.value + sep;
  data += pqCountPerNode.value + sep;
  data += rolaWanFreq.value + sep;
  data += rolaWanDevEui.value + sep;
  data += rolaWanAppEui.value + sep;
  data += rolaWanAppKey.value + sep;
  data += rolaWanDevAddr.value + sep;
  //data += GetSelectedValueByID("nbSimType") + sep;
  if(EN_AGPS.checked)
  	data += EN_AGPS.value;
  else if(DIS_AGPS.checked)
  	data += DIS_AGPS.value;
  data += sep;
  data+=GetSelectedValueByID("custoType1")+sep;
	/*if(mqttdevname1.value==""||mqttdevname1.value==null)
	{	if(parseInt(GetSelectedValueByID("custoType1")>1))
		{	alert("MQTT Device name can not empty.");
		  return
		}
	}*/
	if(mqttdevname1.value == "" || mqttdevname1.value == null)	data += "NaN" + sep;
	else	data+=mqttdevname1.value + sep;
	if(mqttusername1.value == "" || mqttusername1.value == null) data += "NaN" + sep;
	else data+=mqttusername1.value + sep;
	if(mqttpassword1.value == "" || mqttpassword1.value == null) data += window.btoa("NaN") + sep;
	else data+=window.btoa(mqttpassword1.value) + sep;
	if(mqttPubTopic1.value == "" || mqttPubTopic1.value == null) data += "NaN" + sep;
	else data+=mqttPubTopic1.value + sep;
		if(mqttSubTopic1.value == "" || mqttSubTopic1.value == null) data += "NaN" + sep;
	else data+=mqttSubTopic1.value + sep;
	data+=GetSelectedValueByID("custoType2")+sep;
	/*if(mqttdevname2.value==""||mqttdevname2.value==null)
	{	if(parseInt(GetSelectedValueByID("custoType2")>1))
		{	alert("MQTT Device name can not empty.");
		  return
		}
	}*/
	if(mqttdevname2.value == "" || mqttdevname2.value == null) data += "NaN" + sep;
	else data+=mqttdevname2.value + sep;
	if(mqttusername2.value == "" || mqttusername2.value == null) data += "NaN" + sep;
	else data+=mqttusername2.value + sep;
	if(mqttpassword2.value == "" || mqttpassword2.value == null) data += window.btoa("NaN") + sep;
	else data+=window.btoa(mqttpassword2.value) + sep;
	if(mqttPubTopic2.value == "" || mqttPubTopic2.value == null) data += "NaN" + sep;
	else data+=mqttPubTopic2.value + sep;
		if(mqttSubTopic2.value == "" || mqttSubTopic2.value == null) data += "NaN" + sep;
	else data+=mqttSubTopic2.value + sep;
		
	data+=GetSelectedValueByID("nbAgpsSys")+sep;
	data+=GetSelectedValueByID("coldStart")+sep;
	data+=GetSelectedValueByID("hotStart")+sep;
	data+=GetSelectedValueByID("agpsInterval")+sep;
	data+=GetSelectedValueByID("lteAgpsSys")+sep;
	if(suplServ.value=="")	data += "supl.google.com"+sep
	else	data+=suplServ.value+sep;
	if(suplServPort.value=="")	data += "7276"+sep
	else data+=suplServPort.value+sep;
	data += wisunNetName.value + sep;
	data += wisunFreq.value + sep;
	/*if(En_OnBoardWisun.checked)
  	data += "1";
  else if(Dis_OnBoardWisun.checked)
  	data += "0";
  data += sep;*/
  data += otaIp.value + sep;
  data += otaPort.value + sep;
  data += GetCheckedValueById("CYPUMP_YES", "CYPUMP_No", "0") + sep;
  data += initGpsTimeDay.value + sep;
  data += initGpsTimeHour.value + sep;
  /*if(extLvdOn.checked)	data += "1" + sep;
  else									data += "0" + sep;
  if(extLvdLevel.value == "" || extLvdLevel.value == null)	extLvdLevel.value = "0";
  data +=	extLvdLevel.value + sep;*/
  data += "0" + sep;	//ikwAlert1
  data += "0" + sep;	//ikwAlert2
  data += "0" + sep;	//ikwAlert3
  
  var confirmClick=true;
  if(!skip)
  {	if(!extend())
		{	setTimeout("SaveModifiedParam()", 200);
			return;
		}
  	confirmClick = settingAsk();
  }

  if(confirmClick)
  {	if(makeRequest("SaveModifiedParam.cgi", data) == -2)
  	{	setTimeout("SaveModifiedParam(true)", 400);
  	}
  }
  else
  {	extendFinish();
  }
}
function ecrire(line)
{
	showdata.document.writeln(line + "<br>");
}
function PowerSaveSel()
{	var PowerSel = GetSelectedValueByID("pwrSaving");
	
	if(PowerSel >= 1)
	{
		mod_Mobile_interval.disabled = false;
		//DI0_Wakeup.disabled = false;	//board version 2 set this
		//DI1_Wakeup.disabled = false;	//board vwesion 2 set this
		DI0_Wakeup.disabled = true;	//board vwesion 2 remove this 
		DI1_Wakeup.disabled = true;	//board vwesion 2 remove this
		DI2_Wakeup.disabled = false;
		DI3_Wakeup.disabled = false;
    alerton.disabled = false;
	}
	else if(PowerSel == 0)
	{
		mod_Mobile_interval.disabled = false;
		setSelValById("mod_Mobile_interval", "1");
		alerton.disabled = false;
		DI0_Wakeup.disabled = true;
		DI0_Wakeup.checked = false;
		DI1_Wakeup.disabled = true;
		DI1_Wakeup.checked = false;
		DI2_Wakeup.disabled = true;
		DI2_Wakeup.checked = false;
		DI3_Wakeup.disabled = true;
		DI3_Wakeup.checked = false;
	}
}
function parse_resp(rsp)
{
	var tmp, idx, guid_result, time_zone;
	var sts;
	//let authErr = rsp.indexOf("Unauthorized");
	//let tPos = rsp.indexOf("&T=");
	//let tPosE =  rsp.indexOf("\n");
	//var parsed = rsp.substring(tPosE+1, rsp.length).split("\n");
	var parsed = rsp.split("\n");
	//let tt = rsp.substring(tPos+3, tPosE);

	//if(authErr != -1)
	//{	alert("Unauthorized or timeout! \n Re-login");
	//	setTimeout("location.reload()", 1000);
	//	return;
	//}
	//window.sessionStorage.setItem("T", tt);
	data_received = 0;
	if(parsed[0] == '20')
	{	alert('Please reboot A4 Neo for activating the new settings.');
		extendFinish();
	}
	else if(parsed[0] == '24')
	{	alert('AVDS Radar Calibration done!!');
	}
	else if(parsed[0] == '25')
	{	sts = strToInt(parsed[1]);
		if((sts < 3) && (sts > 0))
		{	setTimeout("catchPic(3)", 2000);
			cameraSts.innerHTML = "status: Running ";
		}
		else if(sts == 3)
		{	imgName = parsed[2];
			imgLen = strToInt(parsed[3]);
			imgPos = 0;
			imgBin = "data:image/jpeg;base64,";
			//imgBin = [];
			cameraSts.innerHTML = "status: Get Image";
			getOneImg();
		}
		else if(sts)
		{	alert('Camera Error!!');
			btnGetCloudPic.disabled = false;
			btnCameraCatch.disabled = false;
			btnSendToCloud.disabled = false;
		}
	}
	else if(parsed[0] == '26')
	{	imgBin += parsed[1];
		imgPos += imgDlLen;
		if(imgPos > imgLen)
		{	var img;
			img = document.getElementById("imgId");
			//imgBin = 'data:image/jpeg;base64,' + imgBin;
			img.src = imgBin;
			if(imgMode == 2)
				catchPic(4);
			var sts = "status: Image name:";
			sts += imgName;
			sts += ", length:";
			sts += imgLen;
			sts += " Bytes"
			cameraSts.innerHTML = sts;
			btnGetCloudPic.disabled = false;
			btnCameraCatch.disabled = false;
			btnSendToCloud.disabled = false;
			cameraBrd.click();
			setTimeout("cameraBrd.click()", 400);
		}
		else
			getOneImg();
	}
	else if(parsed[0] == '27')
	{
	}
	else if (parsed[0] == '0')
	{	var cur_parts = parsed[1].split("|");
		setSelValById("mod_rec_sec", cur_parts[0]);
		if(3600 < cur_parts[0])
		{	Year.disabled = true;
			Month.disabled = true;
			Day.disabled = true;
			//Hour.disabled = true;
			gethistory.disabled = true;
		}
		setSelValById("mod_autosend_sec", cur_parts[1]);
		//setSelValById("mod_sleep_times", cur_parts[2]);
		setSelValById("mod_Weak_Up_Interval", cur_parts[2]);
		mod_bat_off.value = cur_parts[3];
		mod_bat_on_diff.value = cur_parts[4];
		setSelValById("pwrSaving", cur_parts[5]);
		PowerSaveSel();
		if(cur_parts[6] == "0") History_record_No.checked = true;
		else 										History_record_Yes.checked = true;
		if(cur_parts[7] == "0") alerton.checked = false;
		else 										alerton.checked = true;
		alerton_level.value = cur_parts[8];
		if(cur_parts[9] == 1)	sewerImEn.checked = true;
		else 										sewerImDis.checked = true;
		/*if(cur_parts[10] == "0")	extLvdOn.checked = false;
		else 											extLvdOn.checked = true;
		extLvdLevel.value = cur_parts[11];*/
		if(cur_parts[12] == 1)	autoFotaEn.checked = true;
		else 										autoFotaDis.checked = true;
		setSelValById("imageServer", cur_parts[13]);
		imgSrvType = cur_parts[13];
		//network
		cur_parts = parsed[2].split("|");
		if(cur_parts[0] == 1)				Mode_LAN.checked = true;
		else if(cur_parts[0] == 2)	Mode_MOB.checked = true;
		mod_ip_addr.value = IntToIPstr(parseInt(cur_parts[1],10));
		mod_ip_mask.value = IntToIPstr(parseInt(cur_parts[2],10));
		mod_ip_gateway.value = IntToIPstr(parseInt(cur_parts[3],10));
		modIpDns.value = IntToIPstr(parseInt(cur_parts[4],10));
		setSelValById("ethMdvpnType", cur_parts[5]);
		otaSel(document.getElementById("ethMdvpnType"), 1);
		cur_parts = parsed[3].split("|");
		if(cur_parts[0] == "4" || cur_parts[0] == "6"|| cur_parts[0] == "7")
			setSelValById("mod_comm_module", "0");
		else
			setSelValById("mod_comm_module", cur_parts[0]);
		if(cur_parts[1] == "6")
			setSelValById("mod_comm_module1", "4");
		else
			setSelValById("mod_comm_module1", cur_parts[1]);
		setHsDisp(document.getElementById("mod_comm_module"))	
  	setLsDisp(document.getElementById("mod_comm_module1"))	
		setSelValById("mod_Mobile_interval", cur_parts[2]);
		setSelValById("lteMdvpnType", cur_parts[3]);
		otaSel(document.getElementById("lteMdvpnType"), 0);
		mod_Senslink_APN.value = cur_parts[4];
		mod_Senslink_APN_NB.value = cur_parts[5];
    mod_Senslink_PLMN.value = cur_parts[6];
		mod_SIMAccount.value = window.atob(cur_parts[7]);		// base64 decoder
		mod_SIMPassword.value = window.atob(cur_parts[8]);	// base64 decoder
		setSelValById("mod_SIMAuthentication", cur_parts[9]);
		if(cur_parts[10] == "1")	En_OnBoardWisun.checked = true;
		else											Dis_OnBoardWisun.checked = true;
		if(En_OnBoardWisun.checked == true)
  		onboardWisunClick(1);
		else if(Dis_OnBoardWisun.checked == true)
			onboardWisunClick(0);
		if (cur_parts[11] == 0) DIS_AGPS.checked = true;
		else 										EN_AGPS.checked = true;
		agpsSel(cur_parts[11]);
		otaIp.value = cur_parts[12];
		otaPort.value = cur_parts[13];
		//AGPS
		cur_parts = parsed[4].split("|");
		setSelValById("nbAgpsSys", cur_parts[0]);
		setSelValById("coldStart", cur_parts[1]);
		setSelValById("hotStart", cur_parts[2]);
		setSelValById("agpsInterval", cur_parts[3]);
		setSelValById("lteAgpsSys", cur_parts[4]);
		suplServ.value = cur_parts[5];
		suplServPort.value = cur_parts[6];
		//mesh
		cur_parts = parsed[5].split("|");
		rolaWanFreq.value = cur_parts[0];
		rolaWanDevEui.value = cur_parts[1];
		rolaWanAppEui.value = cur_parts[2];
		rolaWanAppKey.value = cur_parts[3];
		rolaWanDevAddr.value = cur_parts[4];
		wisunNodeCount.value = cur_parts[5];
		pqCountPerNode.value = cur_parts[6];
		wisunNetName.value = cur_parts[7];
		wisunFreq.value = cur_parts[8];
		GetCurrParam1();
	}
	else if(parsed[0] == '18')
	{	var cur_parts = parsed[1].split("|");
		
		mod_Senslink_Address.value = cur_parts[0];
		mod_Senslink_Port.value = cur_parts[1];
		setSelValById("custoType1", cur_parts[2]);
		setCustoTp1(document.getElementById("custoType1"))		
		mod_Senslink_Address2.value = cur_parts[3];
		mod_Senslink_Port2.value = cur_parts[4];
		setSelValById("custoType2", cur_parts[5]);
		setCustoTp2(document.getElementById("custoType2"))
	  mqttdevname1.value = window.atob(cur_parts[6]);
		mqttusername1.value = window.atob(cur_parts[7]);
		mqttpassword1.value = window.atob(cur_parts[8]);
		mqttPubTopic1.value = window.atob(cur_parts[9]);
    mqttSubTopic1.value = window.atob(cur_parts[10]);
		mqttdevname2.value = window.atob(cur_parts[11]);
		mqttusername2.value = window.atob(cur_parts[12]);
		mqttpassword2.value = window.atob(cur_parts[13]);
		mqttPubTopic2.value = window.atob(cur_parts[14]);
		mqttSubTopic2.value = window.atob(cur_parts[15]);
		cur_parts = parsed[2].split("|");
		if (cur_parts[0] == 0)	Water_level_No.checked = true;
		else 										Water_level_Yes.checked = true;
		if (cur_parts[1] == 0)	GATE_No.checked = true;
		else 										GATE_YES.checked = true;
		if (cur_parts[2] == 0)	PUMP_No.checked = true;
		else 										PUMP_YES.checked = true;
		if (cur_parts[3] == 0)	CYPUMP_No.checked = true;
		else 										CYPUMP_YES.checked = true;
		cyPumpEn(cur_parts[3]);
		setSelValById("sensorType", cur_parts[4]);
		//initLatitude.value = cur_parts[6];
		//initLongitude.value = cur_parts[7];
		initGpsTimeDay.value = cur_parts[5];
		initGpsTimeHour.value = cur_parts[6];
	}
	else if(parsed[0] == '1')
	{	var url;
	  var dbvalue = "";
	  var minute, second;
		tmp = parsed[1] + ".";
		tmp += addZero(parsed[2]) + ".";
		tmp += addZero(parsed[3]) + " ";
		tmp += addZero(parsed[4]) + ":";
		tmp += addZero(parsed[5]) + ":";
		tmp += addZero(parsed[6]);
		//UTC_Date = tmp;
    dvTime.innerHTML = tmp;
		Client_IP.innerHTML = addZero(parsed[7]) + "." + addZero(parsed[8]) + "." + addZero(parsed[9]) + "." + addZero(parsed[10]);
		NB_Client_IP.innerHTML = addZero(parsed[11]) + "." + addZero(parsed[12]) + "." + addZero(parsed[13]) + "." + addZero(parsed[14]);
		var parts = parsed[15].split("|");

    Wireless_state.innerHTML = parts[0];
    NBIOT_state.innerHTML = parts[1];
    
	  Client_IP.innerHTML = Client_IP.innerHTML + parsed[20];
	  NB_Client_IP.innerHTML = NB_Client_IP.innerHTML + parsed[21];
	  if(parsed[22] == '0')
	 		slpRemainTime.innerHTML = "00:00";
	 	else
	 	{	tmp = strToInt(parsed[23]) / 1000;
	 		minute = parseInt(tmp / 60);
	 		second = parseInt(tmp % 60);
	 		dbvalue = addZero(minute) + ":";
	 		dbvalue+= addZero(second)
	 		slpRemainTime.innerHTML = dbvalue;
    }
    currPsmSts.innerHTML = parsed[24];
    initLatitude.value = parsed[25];
    initLongitude.value = parsed[26];
    if((parseFloat(parsed[27]) == 360) && (parseFloat(parsed[28]) == 360))
    {	setNewCoordBtn.disabled = true;
    	currLatitude.value = "";
    	currLongitude.value = "";
    }
    else
		{	setNewCoordBtn.disabled = false;
			currLatitude.value = parsed[27];
    	currLongitude.value = parsed[28];
		}
		locDist.value = parsed[29];
	}
  else if (parsed[0] == '4')
  {
    var strFile = "SensMini A4 Neo " + strToInt(Year.value) + "." + strToInt(Month.value) + "." + strToInt(Day.value) + ".csv";
		var webpage = parsed[1] + "<br>(00 minute)," + parsed[2];
		var csvhour = parsed[1] + "&(00 minute)," + parsed[2];
    showdata = window.open(url, 'recordata');
    showdata.focus();
		csvday = csvday + csvhour;
		webpage = webpage.replace(/&/g, '<br>');
		ecrire(webpage);
		if( parsed[3] < 24)
		{
			GetSDData_Plus(Year.value, Month.value, Day.value, parsed[3]);
	  }
		if(day_count == Count_day.value && parsed[3] == 24)
		{
			csvday = csvday.replace(/&/g, '\r\n');
      download(csvday, strFile, "text/csv");
			csvday = "";
		}
		if(parsed[3] == 24 && day_count < Count_day.value)
		{
				Day.value++;
				day_count++;
				GetSDData_Plus(Year.value, Month.value, Day.value, 0);
		}
  }
  else if (parsed[0] == '5')
  {
		var f2 = parsed[1];
		f2 = f2.replace(/&/g, "<br>");
		showdata = window.open(url, 'parameters');
		ecrire(f2);
  }
	else if(parsed[0] == '7')
	{	var pqNum = parsed[1];
		var maxOnboardPq = parsed[2];
		var parts = parsed[3].split("|");
		var dbvalue = "";
		var objStr;
		var obj;
		
		for(var idx=0;idx<maxOnboardPq;idx++)
		{	objStr = "onboardPq"+idx;
			obj = document.getElementById(objStr);
			if(idx  == 34)
			{	if(parts[34] > 50 || parts[34] < 1)
					obj.innerHTML = "NaN";
				else
					obj.innerHTML = parts[idx].toString() + " V";
			}
			else if(idx  == 35)
			{	if(parts[35] > 50 || parts[35] < 1)
					obj.innerHTML = "NaN";
				else
					obj.innerHTML = parts[idx].toString() + " V";
			}
			else if(idx == 33)
				obj.innerHTML = parts[idx] + "&#8451";
			else
				obj.innerHTML = parts[idx];
		}
		
		createRtPqTb(pqNum);
		for(var i = 0; i < pqNum; i++ )
		{	var pqTdStr;
			var pqTd;
			
			pqTdStr = "rtPq" + i.toString();
			pqTd = document.getElementById(pqTdStr);
			pqTd.innerHTML = parts[strToInt(maxOnboardPq)+i];
		}
  }
	else if(parsed[0] == '8')
	{	Coef_A.value = parsed[1];
		Coef_B.value = parsed[2];
		Coef_C.value = parsed[3];
	}
	else if(parsed[0] == '9')
  {	var objStr, obj;
  	var i, j;
  	setSelValById("COM1_Protocol", parsed[1]);
  	setSelValById("COM2_Protocol", parsed[2]);
  	setSelValById("COM3_Protocol", parsed[3]);
  	setSelValById("ExtDevEth", parsed[4]);
  	setSelValById("ExtDevUsb", parsed[5]);
  	
		for(var i = 1; i < 6; i++)
  	{	j=6+(i-1)*6;
  		objStr = "Device_Enable_" + i;
  		obj = document.getElementById(objStr);
  		if(parsed[j] == 1)	obj.checked = true;	
  		else								obj.checked = false;
  		j++;
  		objStr = "COM_Port_" + i;
  		/*if(obj.checked)
  			SetSelectedValueByID(objStr, strToInt(parsed[j])+1);
  		else*/
  			SetSelectedValueByID(objStr, parsed[j]);
  		j++;
  		objStr = "Command_line_" + i;
  		obj = document.getElementById(objStr);
  		obj.value = parsed[j];
  		j++;
  		objStr = "Start_Byte_" + i;
  		obj = document.getElementById(objStr);
  		obj.value = parsed[j];
  		j++;
  		objStr = "Data_type_" + i;
  		SetSelectedValueByID(objStr, parsed[j]);
  		j++;
  		objStr = "Data_seq_" + i;
  		SetSelectedValueByID(objStr, parsed[j]);
    }
		setComDisp();
    
		if(parsed[36] == 0)	ADC_power_No.checked = true;
		else								ADC_power_Yes.checked = true;
    
		if(parsed[37] == 0)
		{	AI_Diff.checked = true;
			//AI3.disabled = true;
			//AI4.disabled = true;
		}
		else
    {	AI_Single.checked = true;
			//AI3.disabled = false;
			//AI4.disabled = false;
    }
    SetSelectedValueByID("senType0", parsed[38]);
    SetSelectedValueByID("senType1", parsed[39]);
    SetSelectedValueByID("senType2", parsed[40]);
    SetSelectedValueByID("senType3", parsed[41]);
    SetSelectedValueByID("senType4", parsed[42]);
    
    adcEnable(parsed[36]);
  	//SetSelectedValueByID("AI1", parsed[47]);
  	//SetSelectedValueByID("AI2", parsed[48]);
  	//SetSelectedValueByID("AI3", parsed[49]);
  	//SetSelectedValueByID("AI4", parsed[50]);
  	
  	SetSelectedValueByID("ar77_MSEL", parsed[43]);
  	ar77MaxDist.value = parsed[44];
  	ar77MinDist.value = parsed[45];
  	SetSelectedValueByID("ar77_x_range", parsed[46]);
  	SetSelectedValueByID("ar77_resolution", parsed[47]);
  	ar77Offset.value = parsed[48];
  	SetSelectedValueByID("wls_77g", parsed[49]);
  	SetSelectedValueByID("avds_MSEL", parsed[50]);
		//selAvdsMode();
  	avdsStartAzimuth.value = parsed[51];
  	avdsEndAzimuth.value = parsed[52];
  	avdsStartRange.value = parsed[53];
  	avdsEndRange.value = parsed[54];
  	avdsDaulDist.value = parsed[55];
  	avdsTiltRadarAngle.value = parsed[56];
  	avdsTitleRangeBin.value = parsed[57];
  	avdsWaterLevel.value = parsed[58];
  	avdsMaxDist.value = parsed[59];
  	if(parsed[60] == "1")		setTableDisplay("cameraBoard", 1);
  	else										setTableDisplay("cameraBoard", 0);
  	SetSelectedValueByID("osa24gAlert", parsed[61]);
  	setSelValById("osa24gWl", parsed[62]);
  	osaAlertThreshold.value = parsed[63];
  	osaMax.value = parsed[64];
  	osaMin.value = parsed[65];
  	osaSlaveId.value = parsed[66];
  	osaDevNo.value = parsed[67];
  	SetSelectedValueByID("osa24gPwr", parsed[68]);
  	setSelValById("osaPollInt", parsed[69]);
  	selOsaWls();
  	/*
  	* AUDIO
  	*/
  	//smr01WlDevNo.value = parsed[78];
		//smr01Altitude.value = parsed[79];
	
		//smr61WlDevNo.value = parsed[80];
		//smr61RadarDevNo.value = parsed[81];
		//smr61IH.value = parsed[82];
		//smr61Altitude.value = parsed[83];
  	
  	compOsaDevNo.value = parsed[76];
  	setSelValById("compOsaPwr", parsed[77]);
  	compWlsDevNo.value = parsed[78];
  	setSelValById("compWlsPwr", parsed[79]);
  	compOsaAlertTrig.value = parsed[80];
  	compOsaAlertRet.value = parsed[81];
  	compOsaIh.value = parsed[82];
  	compOsaDist.value = parsed[83];
  	compOsaAlt.value = parsed[84];
  	
  	setSelValById("compLr110Ai", parsed[85]);
  	setSelValById("compLR110Pwr", parsed[86]);
  	setSelValById("compLr110WlsAiNo", parsed[87]);
  	setSelValById("compLr110WlsPwr", parsed[88]);
  	compLr110AlertTrig.value = parsed[89];
  	compLr110AlertRet.value = parsed[90];
  	compLr110Ih.value = parsed[91];
  	compLr110Dist.value = parsed[92];
  	compLr110Alt.value = parsed[93];
  	compLr110Blind.value = parsed[94];
  	setSelValById("rs485baudrate_sel", parsed[95]);
		SetSelectedValueByID("rs485PDS_sel", parsed[96]);
		setSelValById("rs485_2baudrate_sel", parsed[97]);
		SetSelectedValueByID("rs485_2PDS_sel", parsed[98]);
		setSelValById("rs232_1baudrate_sel", parsed[99]);
		setSelValById("rs232_1PDS_sel", parsed[100]);
  	setSelValById("mod_camera_interval", parsed[101]);
  	setSelValById("cameraAlertInterval", parsed[102]);
		setSelValById("cameraResolution", parsed[103]);
		setSelValById("cameraQuality", parsed[104]);

		vwDevNo.value = parsed[105];
		vwSlaveId.value = parsed[106];
		setSelValById("vwPwr", parsed[107]);
		vwPwrTime.value = parsed[108];
		setSelValById("vwPollInt", parsed[109]);
		if(parsed[110] == 0) 	di_status_No.checked = true;
		else 									di_status_Yes.checked = true;
		if(parsed[111] == 1) 	DI0_Wakeup.checked = true;
		else 									DI0_Wakeup.checked = false;
		if(parsed[112] == 1) 	DI1_Wakeup.checked = true;
		else 									DI1_Wakeup.checked = false;
		setSelValById("dynamicRecSec", parsed[113]);
		if(parsed[114] == 1) 	DI2_Wakeup.checked = true;
		else 									DI2_Wakeup.checked = false;
		if(parsed[115] == 1) 	DI3_Wakeup.checked = true;
		else 									DI3_Wakeup.checked = false;
  }
  else if(parsed[0] == '6')
  {
    var node_parts = parsed[2].split("|");
  	var j = 0;
	  for(var i = 0; i <= 7; i++)
	  {
	  	j = i*7;
	  	eval('if(node_parts['+ j +'] == 1) {Channel_Enable_'+ i +'.checked = true;}	else	{Channel_Enable_'+ i +'.checked = false;}');
	  	j = j + 1;
	  	eval('Device_no_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('Item_num_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('EQN_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('Filter_Enable_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('Alias_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('srv2Alias_'+ i +'.value = node_parts['+ j +'];');
	  	/*eval('if(node_parts['+ j +'] == 1) {Power'+ i + '_DO_0.checked = true;}	else {Power'+ i +'_DO_0.checked = false;}');
	  	j = j + 1;
	  	eval('if(node_parts['+ j +'] == 1) {Power'+ i + '_DO_1.checked = true;}	else {Power'+ i +'_DO_1.checked = false;}');*/
	  }
		if(parsed[1] == '14')
			GetNodeParam1();
  }
  else if(parsed[0] == '17')
  {
  	var node_parts = parsed[1].split("|");
  	/*var j = 0;
	  for(i = 0; i < 4; i++)
	  {
	  	j = i*5;
	  	eval('if(node_parts['+ j +'] == 1) {DO_Enable_'+ i +'.checked = true;}	else	{DO_Enable_'+ i +'.checked = false;}');
	  	j = j + 1;
	  	eval('Software_no_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('Bat_low_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('Bat_high_'+ i +'.value = node_parts['+ j +'];');
	  	j = j + 1;
	  	eval('SetSelectedValueByID("DO_Mode_'+ i +'", node_parts['+ j +']);');
	  }*/
	  if(node_parts[0] == 1) {DO_0_Output.checked = true;}	else	{DO_0_Output.checked = false;}
	  if(node_parts[1] == 1) {DO_1_Output.checked = true;}	else	{DO_1_Output.checked = false;}
	  SetSelectedValueByID("DO_Master_Mode_0", node_parts[2]);
	  if(node_parts[3] == 1) {DO_5V_Output.checked = true;}	else	{DO_5V_Output.checked = false;}
	  SetSelectedValueByID("doMapSel0", node_parts[4]);
	  SetSelectedValueByID("doMapSel1", node_parts[5]);
	  SetSelectedValueByID("doMapSel2", node_parts[6]);
	  SetSelectedValueByID("doMapSel3", node_parts[7]);
  }
  else if(parsed[0] == '10')
  {
     if(parsed[1] == 1)
     {
        login_ok = 1;
        isSu = parsed[2];
        document.form1.autorun.value = "";
    		document.form1.power.value = "";
    		document.form1.new_username.value = "";
    		document.form1.new_password.value = "";
    		document.form1.confirm_password.value = "";
        mod_reset.disabled = false;
    		document.form1.new_username.disabled = false;
    		document.form1.new_password.disabled = false;
    		document.form1.confirm_password.disabled = false;
        document.getElementsByTagName("span")[0].style.color="black";
        document.getElementsByTagName("span")[1].style.color="blue";
        document.getElementsByTagName("span")[2].style.color="black";
        document.getElementsByTagName("span")[3].style.color="black";
        document.getElementsByTagName("span")[4].style.color="black";
        document.getElementsByTagName("span")[5].style.color="black";
        Tab0.style.display='none';
    		Tab1.style.display='';
    		Tab2.style.display='none';
    		Tab3.style.display='none';
    		Tab4.style.display='none';
        Tab5.style.display='none';
    		UpdateTime();
        GetCurrParam();
     }
     else if(parsed[1] == 2)
     {	isSu = 0;
    	 	alert("Username/password error!");
    	 	login_ok = 0;
    	 	mod_reset.disabled = true;
    		document.form1.autorun.value = "";
    		document.form1.power.value = "";
    		document.form1.new_username.disabled = true;
    		document.form1.new_password.disabled = true;
    		document.form1.confirm_password.disabled = true;
     }
  	TCP_Status = parsed[3];
  	if(parsed[3] == "ini_NG")
  	{
  		alert("Not load parameter.ini correctly. Turn to load default parameters.");
  	}
  	else if(parsed[2] == "TCP_NG")
  	{
  		alert("TCP Server settings NG. Turn to load default parameters.");
  	}
  }
  else if(parsed[0] == '11')
  {
  	var guid_tmp;
  	var guid_transfer = new Array();
  	fwVersion.innerHTML = parseInt(parsed[1],10).toString(16).toUpperCase();
  	/*if(parsed[18] == 1)  fwVersion.innerHTML = fwVersion.innerHTML + "(Senstalk2/";
    else fwVersion.innerHTML = fwVersion.innerHTML + "(Senstalk1/";
  	if(parsed[19] == 1)  fwVersion.innerHTML = fwVersion.innerHTML + "UTC-";
    else fwVersion.innerHTML = fwVersion.innerHTML + "Local time-";
  	if(parsed[20] == 1)  fwVersion.innerHTML = fwVersion.innerHTML + "ID";
    else fwVersion.innerHTML = fwVersion.innerHTML + "RN";
		if(parsed[21] == 1)  fwVersion.innerHTML = fwVersion.innerHTML + "/BETA version)";
		else	fwVersion.innerHTML = fwVersion.innerHTML + ")";*/
	fwVersion.innerHTML = fwVersion.innerHTML + parsed[18];
  	guid_result = "";
  	for(var num_guid = 0; num_guid < 4; num_guid++)
  	{
  		guid_transfer[3-num_guid] = parsed[num_guid+2];
  	}
  	for(var num_guid = 4; num_guid < 6; num_guid++)
  	{
  		guid_transfer[9-num_guid] = parsed[num_guid+2];
  	}
  	for(var num_guid = 6; num_guid < 8; num_guid++)
  	{
  		guid_transfer[13-num_guid] = parsed[num_guid+2];
  	}
   	for(var num_guid = 8; num_guid < 16; num_guid++)
  	{
  		guid_transfer[num_guid] = parsed[num_guid+2];
  	}
  	for(var num_guid = 0; num_guid < 16; num_guid++)
  	{
  		guid_tmp = parseInt(guid_transfer[num_guid],10).toString(16);
  		if(guid_tmp.length == 1) guid_tmp = '0' + guid_tmp;
  		guid_result = guid_result + guid_tmp;
  		if(num_guid == 3 || num_guid == 5 || num_guid == 7 || num_guid == 9)
  		{
  			guid_result = guid_result + '-';
  		}
  	}
  	M4Guid.innerHTML = guid_result;
  	if(imgGetGuid)
  	{	var api;
  		var img;
  		const pcTimezone = new Date().getTimezoneOffset();
  		const pcTimezone1 = -pcTimezone;
			img = document.getElementById("imgId");
			api = "https:/";
			if(imgSrvType == 1)
				api += "/device.senslink.id/v3/sensmini/Images/";
			else
  			api += "/device.senslink.net/v3/sensmini/Images/";
  		api += guid_result;
  		api += "?tag=Default_station";
  		api += imgIdx;
  		//api += "&timeZone=480";
  		api += "&timeZone="+pcTimezone1.toString();
  		imgIdx++;
  		imgGetGuid = 0;
  		img.src = api;
  		setTableDisplay("cameraSts", 1);
  		cameraSts.innerHTML = api;
  		btnGetCloudPic.disabled = false;
			btnCameraCatch.disabled = false;
			btnSendToCloud.disabled = false;
			cameraBrd.click();
			setTimeout("cameraBrd.click()", 800);
  	}
  }
  else if(parsed[0] == '12')
  {
  	Coef_A_1.value = parsed[1];
  	Coef_B_1.value = parsed[2];
  }
  else if(parsed[0] == '13')
  {
		var webpage1 = parsed[1];
    showdata = window.open(url, 'recordata');
    showdata.focus();
    webpage1 = webpage1.replace(/&/g, '<br>');
    if(parsed[1] != '13')	showdata.document.writeln(webpage1);
  }
  /*else if(parsed[0] == '19')
  {	var strFile = "SensMini A4 Neo_Log.log";
		var webpage1 = parsed[1];
		var webData = parsed[1];
    showdata = window.open(url, 'recordata');
    showdata.focus();
    webpage1 = webpage1.replace(/&/g, '<br>');
    if(parsed[1] != '19')	showdata.document.writeln(webpage1);
    webData = webData.replace(/&/g, '\r\n');
    download(webData, strFile, "text/txt");
  } */
}

function GetCurrParam1()
{	var sep = "|";
	var data = "2" + sep;
	if(makeRequest("GetCurrParam.cgi", data) == -2)
	{	setTimeout("GetCurrParam1()", 400);
	}
}

function GetCurrParam()
{
	var sep = "|";
	var data = "1" + sep;
	if(makeRequest("GetCurrParam.cgi", data) == -2)
	{	setTimeout("GetCurrParam()", 400);
		return;
	}
	UpdateTime();
	clearTimeout(RTValue);
}
function ResetDevice(skip=false)
{	var confirmClick = true;

	if(!skip)
	{	if(!extend())
		{	setTimeout("ResetDevice()", 200);
			return;
		}
		confirmClick = confirm('Please reload the web page in 5 seconds.');
	}
	if(confirmClick)
	{	if(makeRequest("ResetDevice.cgi", 0) == -2)
  		setTimeout("ResetDevice(true)", 400);
  	else
  		setTimeout("location.reload()", 1000);
	}
	else
		extendFinish();
}
function UpdateTime()
{
	localTime.innerHTML=GetDTString();
	if (!data_received)
		makeRequest("GetDeviceTime.cgi",0);
	if(DtTimerStop == false)
	DTTimer=setTimeout("UpdateTime()", 1000);
	DtTimerStop = false;
}
function SyncDateTime()
{
	var time_set = new Date();
	var data = "";
	var sep = "|";
  	var month = time_set.getUTCMonth() + 1;
  	data += time_set.getUTCFullYear()           + sep;
  	data += addZero(month)                      + sep;
  	data += addZero(time_set.getUTCDate())      + sep;
  	data += addZero(time_set.getUTCHours())     + sep;
  	data += addZero(time_set.getUTCMinutes())   + sep;
  	data += addZero(time_set.getUTCSeconds());
	if(makeRequest("SetDeviceTime.cgi", data) == -2)
	{	setTimeout("SyncDateTime()", 400);
	}
}
/*function SyncDateTime1()
{
	var time_set = new Date();
	var data = "";
	var sep = "|";
  	var month = time_set.getMonth() + 1;
  	data += time_set.getFullYear()          + sep;
  	data += addZero(month)                  + sep;
  	data += addZero(time_set.getDate())     + sep;
  	data += addZero(time_set.getHours())    + sep;
  	data += addZero(time_set.getMinutes())  + sep;
  	data += addZero(time_set.getSeconds());
	if(makeRequest("SetDeviceTime1.cgi", data) == -2)
	{	setTimeout("SyncDateTime1()", 400);
	}
}*/

function download(strData,strFileName,strMimeType)
{
	var D=document,A=arguments,a=D.createElement("a"),d=A[0],n=A[1],t=A[2]||"text/plain";
	a.href="data:"+strMimeType+","+escape(strData);
	if('download'in a)
	{
		a.setAttribute("download",n);
		a.innerHTML="downloading...";
		D.body.appendChild(a);
		setTimeout(function()
		{
			var e=D.createEvent("MouseEvents");
			e.initMouseEvent("click",true,false,window,0,0,0,0,0,false,false,false,false,0,null);
			a.dispatchEvent(e);
			D.body.removeChild(a);
		},66);
		return true;
	};
	var f=D.createElement("iframe");
	D.body.appendChild(f);
	f.src="data:"+(A[2]?A[2]:"application/octet-stream")+(window.btoa?";base64":"")+","+(window.btoa?window.btoa:escape)(strData);
	setTimeout(function(){D.body.removeChild(f);},333);
	return true;
}
function ResetParam(skip=false)
{	var sep = "|";
	var data = "4" + sep;
	var confirmClick=true;
	if(!skip)
	{	if(!extend())
		{	setTimeout("ResetParam()", 200);
			return;
		}
		confirmClick = confirm('After resetting, please reload the web page in 5 seconds.');
	}
	if(confirmClick)
	{	if(makeRequest("GetSDData.cgi", data) == -2)
		{	setTimeout("ResetParam(true)", 400);
		}
		else
			setTimeout("location.reload()", 1000);
	}
	else
		extendFinish();
}

function setCustoTp2(selObj)
{	var val = selObj.value;
	var svr1Val = GetSelectedValueByID("custoType1");
	if((val <= 2) || (val == 5) || ((val >= 7) && (val <= 10)))
	{	setTableDisplay("Mqtt2Row2", 1);
		setTableDisplay("Mqtt2Row3", 1);
		mqtt1Name.innerHTML = "MQTT Username";
		if(val == 5)
		{	setTableDisplay("Mqtt2Row1", 1);
			setTableDisplay("Mqtt2Row4", 1);
			setTableDisplay("Mqtt2Row5", 1);
			mqtt1CliId.innerHTML = "MQTT Device name / Client Id";
		}
		else
		{	setTableDisplay("Mqtt2Row1", 0);
			setTableDisplay("Mqtt2Row4", 0);
			setTableDisplay("Mqtt2Row5", 0);
		}
		setTableDisplay("srv2Board", 1);
	}
	else if(val == 100)
	{	setTableDisplay("Mqtt2Row1", 1);
		setTableDisplay("Mqtt2Row2", 1);
		mqtt1CliId.innerHTML = "IOA EQUIP ID";
		mqtt1Name.innerHTML = "IOA API KEY";
		setTableDisplay("Mqtt2Row3", 0);
		setTableDisplay("Mqtt2Row4", 0);
		setTableDisplay("Mqtt2Row5", 0);
		setTableDisplay("srv2Board", 1);
	}
	else
	{	setTableDisplay("Mqtt2Row1", 0);
		setTableDisplay("Mqtt2Row2", 0);
		setTableDisplay("Mqtt2Row3", 0);
		setTableDisplay("Mqtt2Row4", 0);
		setTableDisplay("Mqtt2Row5", 0);
		setTableDisplay("srv2Board", 0);
		if(srv2Panel.style.maxHeight != null && srv2Panel.style.maxHeight != 0)
			srv2Board.click();
	}

	if((val == 9) || (val == 10) || (svr1Val == 9) || (svr1Val == 10))
	{	setTableDisplay("sewerPanel", 1);
		setTableDisplay("sewerChgStaPanel", 1);
		setTableDisplay("sewerChgSensPanel", 1);
		//setTableDisplay("sewerImPanel", 1);
	}
	else
	{	setTableDisplay("sewerPanel", 0);
		setTableDisplay("sewerChgStaPanel", 0);
		setTableDisplay("sewerChgSensPanel", 0);
		//setTableDisplay("sewerImPanel", 0);
	}
}

function setCustoTp1(selObj)
{	var val = selObj.value;
	var svr2Val = GetSelectedValueByID("custoType2");
	
	if((val <= 2) || (val == 5) || ((val >= 7) && (val <= 10)))
	{	setTableDisplay("Mqtt1Row2", 1);
		setTableDisplay("Mqtt1Row3", 1);
		mqtt0Name.innerHTML = "MQTT Username";
		if(val == 5)
		{	setTableDisplay("Mqtt1Row1", 1);
			setTableDisplay("Mqtt1Row4", 1);
			setTableDisplay("Mqtt1Row5", 1);
			mqtt0CliId.innerHTML = "MQTT Device name / Client Id";
		}
		else
		{	setTableDisplay("Mqtt1Row1", 0);
			setTableDisplay("Mqtt1Row4", 0);
			setTableDisplay("Mqtt1Row5", 0);
		}
		setTableDisplay("srv1Board", 1);
	}
	else if(val == 100)
	{	setTableDisplay("Mqtt1Row1", 1);
		setTableDisplay("Mqtt1Row2", 1);
		mqtt0CliId.innerHTML = "IOA EQUIP ID";
		mqtt0Name.innerHTML = "IOA API KEY";
		setTableDisplay("Mqtt1Row3", 0);
		setTableDisplay("Mqtt1Row4", 0);
		setTableDisplay("Mqtt1Row5", 0);
		setTableDisplay("srv1Board", 1);
	}
	else
	{	setTableDisplay("Mqtt1Row1", 0);
		setTableDisplay("Mqtt1Row2", 0);
		setTableDisplay("Mqtt1Row3", 0);
		setTableDisplay("Mqtt1Row4", 0);
		setTableDisplay("Mqtt1Row5", 0);
		setTableDisplay("srv1Board", 0);
		if(srv1Panel.style.maxHeight != null && srv1Panel.style.maxHeight != 0)
			srv1Board.click();
	}
	
	if((val == 9) || (val == 10) || (svr2Val == 9) || (svr2Val == 10))
	{	setTableDisplay("sewerPanel", 1);
		setTableDisplay("sewerChgStaPanel", 1);
		setTableDisplay("sewerChgSensPanel", 1);
		//setTableDisplay("sewerImPanel", 1);
	}
	else
	{	setTableDisplay("sewerPanel", 0);
		setTableDisplay("sewerChgStaPanel", 0);
		setTableDisplay("sewerChgSensPanel", 0);
		//setTableDisplay("sewerImPanel", 0);
	}
}

function setHsDisp(selObj)
{	var val = selObj.value;
	var lsVal = GetSelectedValueByID("mod_comm_module1");
	
	if(val == 9)
	{	if(lsVal == 9)
		{	SetSelectedValueByID("mod_comm_module", 0);
			setTableDisplay("meshBoard", 0);
			if(meshPanel.style.maxHeight != null && meshPanel.style.maxHeight != 0)
				meshBoard.click();
			return;
		}
		setTableDisplay("loraWanBoard", 1);
	}
	else
	{	if(lsVal != 9)
		{	setTableDisplay("loraWanBoard", 0);
			if(loraWanPanel.style.maxHeight != null && loraWanPanel.style.maxHeight != 0)
				loraWanBoard.click();
		}
		
		if((lsVal != 8) && (lsVal != 10) && Dis_OnBoardWisun.checked == true)
		{	setTableDisplay("meshBoard", 0);
			if(meshPanel.style.maxHeight != null && meshPanel.style.maxHeight != 0)
				meshBoard.click();
		}
	}
}

function setLsDisp(selObj)
{	var val = selObj.value;
	var hsVal = GetSelectedValueByID("mod_comm_module");
	
	if(val == 9)
	{	if(hsVal == 9)
		{	SetSelectedValueByID("mod_comm_module1", 0);
			setTableDisplay("meshBoard", 0);
			if(meshPanel.style.maxHeight != null && meshPanel.style.maxHeight != 0)
				meshBoard.click();
			return;
		}
		setTableDisplay("loraWanBoard", 1);
		setTableDisplay("meshBoard", 0);
		if(meshPanel.style.maxHeight != null && meshPanel.style.maxHeight != 0)
			meshBoard.click();
	}
	else
	{	if(hsVal != 9)
		{	setTableDisplay("loraWanBoard", 0);
			if(loraWanPanel.style.maxHeight != null && loraWanPanel.style.maxHeight != 0)
				loraWanBoard.click();
		}
		
		if((val == 8) || (val == 10) || En_OnBoardWisun.checked == true)
			setTableDisplay("meshBoard", 1);
		else
		{	setTableDisplay("meshBoard", 0);
			if(meshPanel.style.maxHeight != null && meshPanel.style.maxHeight != 0)
				meshBoard.click();
		}
	}
}

function setComDisp()
{	var com1Sel = GetSelectedValueByID("COM1_Protocol");
	var com2Sel = GetSelectedValueByID("COM2_Protocol");
	var UsbSel = GetSelectedValueByID("ExtDevUsb");
	/*if((com1Sel == 20) || (com2Sel == 21))
		setTableDisplay("AR77_board", 1);
	else
	{	setTableDisplay("AR77_board", 0);
		if(ar77Panel.style.maxHeight != null && ar77Panel.style.maxHeight != 0)
			AR77_board.click();
	}*/
	if((com1Sel == 4) || (com2Sel == 4))
	{	setTableDisplay("AVDS_board", 1);
		selAvdsMode();
	}
	else
	{	setTableDisplay("AVDS_board", 0);
		if(avdsPanel.style.maxHeight != null && avdsPanel.style.maxHeight != 0)
			AVDS_board.click();
	}
	if(UsbSel == 7)
		setTableDisplay("cameraBrd", 1);
	else
	{	setTableDisplay("cameraBrd", 0);
		if(cameraPanel.style.maxHeight != null && cameraPanel.style.maxHeight != 0)
			cameraBrd.click();
	}
		
	if(isComModbus(1) || isComModbus(2) || isComModbus(3))
	{	setTableDisplay("OSA24_board", 1);
		//setTableDisplay("compOsaBoard", 1);
		setTableDisplay("vw_board", 1);
	}
	else
	{	setTableDisplay("OSA24_board", 0);
		setTableDisplay("compOsaBoard", 0);
		setTableDisplay("vw_board", 0);
		if(osa24Panel.style.maxHeight != null && osa24Panel.style.maxHeight != 0)
			OSA24_board.click();
		if(compOsa24Panel.style.maxHeight != null && compOsa24Panel.style.maxHeight != 0)
			compOsaBoard.click();
		if(vwPanel.style.maxHeight != null && vwPanel.style.maxHeight != 0)
			vw_board.click();
	}
}

function selAvdsMode()
{	var sel = GetSelectedValueByID("avds_MSEL");
	if(sel == "0")
	{	setTableDisplay("dualSet1", 0);
		setTableDisplay("dualSet2", 0);
		setTableDisplay("dualSet3", 0);
		setTableDisplay("dualSet4", 0);
		setTableDisplay("btnCalibration", 0);
	}
	else
	{	if(sel == "1")
		{	setTableDisplay("btnCalibration", 1);
			setTableDisplay("dualSet4", 0);
		}
		else
		{	setTableDisplay("btnCalibration", 0);
			setTableDisplay("dualSet4", 1);
		}
		setTableDisplay("dualSet1", 1);
		setTableDisplay("dualSet2", 1);
		setTableDisplay("dualSet3", 1);
	}
	AVDS_board.click();
	setTimeout("AVDS_board.click()", 400);
}

function agpsSel(sel)
{	if(sel == 1)
	{	setTableDisplay("agpsBoard", 1);
		//setTableDisplay("me310SysRow", 1);
		//setTableDisplay("me310ColdRow", 1);
		//setTableDisplay("me310HotRow", 1);
		//setTableDisplay("me310PollRow", 1);
		//setTableDisplay("le910SysRow", 1);
		//setTableDisplay("le910SuplRow", 1);
		//setTableDisplay("le910SuplPortRow", 1);
	}
	else if(sel == 0)
	{	//setTableDisplay("me310SysRow", 0);
		//setTableDisplay("me310ColdRow", 0);
		//setTableDisplay("me310HotRow", 0);
		//setTableDisplay("me310PollRow", 0);
		//setTableDisplay("le910SysRow", 0);
		//setTableDisplay("le910SuplRow", 0);
		//setTableDisplay("le910SuplPortRow", 0);
		setTableDisplay("agpsBoard", 0);
		if(agpsPanel.style.maxHeight != null && agpsPanel.style.maxHeight != 0)
			agpsBoard.click();
	}
}

function temporarySleep(enabled)
{	var sep = "|";
	var data;
	var reqSts;
	
	if(enabled)
		data = "1";
	else
		data = "0";
	data += sep;
	if(makeRequest("temporaryDisSlp.cgi", data) == -2)
  {	if(enabled)
  		setTimeout("temporarySleep(1)", 400);
  	else
  		setTimeout("temporarySleep(0)", 400);
  }
}

function onboardWisunClick(enabled)
{	var lsVal = GetSelectedValueByID("mod_comm_module1");
	
	if(enabled)
	{	if(lsVal == 8)
		{	SetSelectedValueByID("mod_comm_module1", 0);
			return;
		}
		setTableDisplay("wisunMainRow", 1);
		setTableDisplay("loraNodesRow", 1);
		setTableDisplay("loraPqsRow", 1);
		setTableDisplay("wisunNetNameRow", 1);
		setTableDisplay("wisunFreqRow", 1);
	}
	else
	{	if(lsVal == 8)
		{	return;
		}
		setTableDisplay("wisunMainRow", 0);
		setTableDisplay("loraNodesRow", 0);
		setTableDisplay("loraPqsRow", 0);
		setTableDisplay("wisunNetNameRow", 0);
		setTableDisplay("wisunFreqRow", 0);
	}
}

function otaSel(selObj, isWired)
{	var val = selObj.value;
	var otherVal;
	
	if(isWired == 1)
		otherVal = GetSelectedValueByID("lteMdvpnType");
	else
		otherVal = GetSelectedValueByID("ethMdvpnType");
		
	if((val == 2) || (otherVal == 2))
		setTableDisplay("otaBoard", 1);
	else
	{	setTableDisplay("otaBoard", 0);
		if(otaPanel.style.maxHeight != null && otaPanel.style.maxHeight != 0)
			otaBoard.click();
	}
}

function changeSta()
{	var data = "1|";
	
	if(makeRequest("chgSta.cgi", data) == -2)
  {	setTimeout("changeSta(1)", 400);
  }
}

function chgSensor()
{	var data;
	
	data = GetSelectedValueByID("sensorType") + "|";
	if(makeRequest("chgSensor.cgi", data) == -2)
  {	setTimeout("chgSensor()", 400);
  }
}

function cyPumpEn(sel)
{	if(sel == 1)
	{	setTableDisplay("cyPumpBoard", 1);
	}
	else
	{	setTableDisplay("cyPumpBoard", 0);
		if(cyPumpPanel.style.maxHeight != null && cyPumpPanel.style.maxHeight != 0)
			cyPumpBoard.click();
	}
}


function setNewCoord(sel)
{	var data;
	
	data = currLatitude.value + "|" + currLongitude.value + "|";
	if(makeRequest("setNewCoord.cgi", data) == -2)
  {	setTimeout("setNewCoord()", 400);
  }
}