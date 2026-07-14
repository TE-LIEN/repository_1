function Tab4Clicked()
{
	document.getElementsByTagName("span")[0].style.color="black";
  document.getElementsByTagName("span")[1].style.color="black";
  document.getElementsByTagName("span")[2].style.color="black";
  document.getElementsByTagName("span")[3].style.color="black";
  document.getElementsByTagName("span")[4].style.color="blue";
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
		Tab2.style.display='none';
		Tab3.style.display='none';
		Tab4.style.display='';	
		Tab5.style.display='none';	
	  UpdateRealData();		
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
function UpdateRealData()
{
  if(makeRequest("GetRealValue.cgi", 0) == -2)
  {	setTimeout("UpdateRealData()", 400);
  }
  else
  	RTValue = setTimeout("UpdateRealData()", 5000);		
}
function GetSDData_Plus(year, month, day, hour)
{
	var sep = "|";
	var data = "1" + sep;
	//var data = "3" + sep;
	data += strToInt(year) + sep;
	data += strToInt(month) + sep;
	data += strToInt(day) + sep;
	data += strToInt(hour);

	if(makeRequest("GetSDData.cgi", data) == -2)
	{	setTimeout("GetSDData_Plus()", 400);
	}
}
function GetSDData_Days()
{
		day_count = 0;
		month_num = strToInt(Month.value);
		day_num = strToInt(Day.value);
		count_num = strToInt(Count_day.value);
		if(month_num == 1 || month_num == 3 || month_num == 5 || month_num == 7 || month_num == 8 || month_num == 10 || month_num == 12)
		{
			if((day_num + count_num - 1) > 31)
			{
				alert("Total day(s) number is too large for this month.");
			}
			else
			{
				GetSDData_Plus(Year.value, Month.value, Day.value, 0);
				day_count++;
			}
		}
		if(month_num == 4 || month_num == 6 || month_num == 9 || month_num == 11)
		{
			if((day_num + count_num - 1) > 30)
			{
				alert("Total day(s) number is too large for this month.");
			}
			else
			{
				GetSDData_Plus(Year.value, Month.value, Day.value, 0);
				day_count++;
			}
		}
		if(month_num == 2)
		{
			if((day_num + count_num - 1) > 29)
			{
				alert("Total day(s) number is too large for this month.");
			}
			else
			{
				GetSDData_Plus(Year.value, Month.value, Day.value, 0);
				day_count++;
			}
		}	
}
function GetParamFile()
{
	var sep = "|";
	var data = "2" + sep;
	if(makeRequest("GetSDData.cgi", data) == -2)
	{	setTimeout("GetParamFile()", 400);
	}
}

function createRtPqTb(pqNum)
{	var tr, td;
	var idx;
	var rowIdx;
	var pqRowCnt = 4;
	var currIdx;
	
	if(pqTbCreated)
		return;
	pqTbCreated = 1;
	for(idx=0;idx<pqNum;idx+=pqRowCnt)
	{	rowIdx = 0;
		tr = document.createElement("tr");
		for(rowIdx=0;rowIdx<pqRowCnt;rowIdx++)
		{	currIdx = idx+rowIdx;
			if(currIdx >= pqNum)
				break;
			td = document.createElement("td");
			td.classList.add("pqRtIdx");
			td.innerHTML = padZero(currIdx.toString(), 2);
			tr.appendChild(td);

			td = document.createElement("td");
			td.classList.add("pqRtVal");
			td.id = "rtPq" + currIdx.toString();
			tr.appendChild(td);
		}
		rtPqTb.appendChild(tr);
	}
}