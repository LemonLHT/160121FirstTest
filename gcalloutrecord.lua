
require "luasql.odbc"
require "config"


env = ni
conMPUser = nil
curCheckOutData = nil

--data source info
dsnName = MC_DB_DSNNAME
--print(dsnName)
userName = MC_DB_USERNAME
passWord = MC_DB_PASSWORD
tableNmae = MC_DB_DATANAME
RecordName   = MC_DB_LOGNAME
--path
filestorepath = MC_FILEPATH
filebakpath = MC_FILEBAKPATH
filerejectpath = MC_FILEREJECTPATH
--golbal
callerFileName = nil
stagLogo = STAGELogo
filenameprefix = FILENAMEPREFIX
g_dataCount = 0
stageinfo = STAGELOGOINFO
g_indexrecord = nil


function main()

	print("Start lua..")
	--连接数据库
	initDB()
	--初始化路径
	initPath()
	--得到全局的计数标识
	if GetTheIndex() ~= true then
		return
	end
	--
	GetFileName()
	--判断
	--today = tonumber(os.date("%d"))
	--print(today)
	-- 如果是1号 出全量
	if judgeDay() == true then
		getfulldata()
	else
		getAdddata()
	end

	print("END")
end

function getfulldata()
	print ("fulldata")
	--得到上一个月的最后一天的日期
	local y = tonumber(os.date("%Y"))
	local m = tonumber(os.date("%m"))
	local lastmonth = m -1
	if lastmonth == 0 then
		lastmonth = 12
		y = y -1
	end

	local lastMothDays = os.date("%d", os.time({year=os.date("%Y"),month=os.date("%m"),day=0}))
	local fulltime = string.format("%04d-%02d-%02d 23:59:59",y,lastmonth,lastMothDays)
	print(fulltime)
	stQueryFullData(fulltime)

end
-- 创建路径
function initPath()
	print (filestorepath)
	local bExist = os.execute('cd ' ..filestorepath )
	if bExist == 1 then
		os.execute('mkdir ' .. filestorepath)
	end
	bExist = os.execute('cd ' ..filebakpath )
	if bExist == 1 then
		os.execute('mkdir ' .. filebakpath)
	end
	bExist = os.execute('cd ' ..filerejectpath )
	if bExist == 1 then
		os.execute('mkdir ' .. filerejectpath)
	end
end
--初始化数据库
function initDB()
	env = luasql.odbc()
	if env == nil then
		C_PrintLog("Load luasql failed");
		--print("Load luasql fialed ")
		return false
	end
	-- link the datasource
	--print(dsnName,userName,passWord)
	conMPUser = env:connect(dsnName,userName,passWord)
	if nil == conMPUser then
		C_PrintLog("Connect to bill failed")
		--print ("connect to bil failed")
		return false
	end
	return true
end
--获得增量。
function getAdddata()
	--如果不是1号出增量
	year = os.date("%Y")
	mon  = os.date("%m")
	lastday  = os.date("%d") -1
	yesterdayM = string.format("%04d-%02d-%02d 00:00:00",year,mon,lastday)
	yesterdayA = string.format("%04d-%02d-%02d 23:59:59",year,mon,lastday)
	--print (yestaday)
	-- select the yesterday data from datasource
	stQueryAddData(yesterdayM,yesterdayA)

end
--获得全量数据。
function stQueryFullData(lastday)
	print("-----------get the full data ----------------")
	local sql = string.format("select distinct B_CALLER as BCALLER FROM %s where to_char(B_TIME,\'yyyy-mm-dd hh24:mi:ss\') <= \'%s\' and B_STATUS =2",tableNmae,lastday)
	--插入的记录表中进行记录以及去除已经有过的外呼数据。以便于本次数据的读取。
	local mergesql = string.format("merge into %s T1 using (%s) T2 on (T1.CC_CALLER = T2.BCALLER) when not matched then insert (CC_CALLER, CC_INDEXFLAG) VALUES(T2.BCALLER, %d)",RecordName,sql,g_indexrecord)
	local Res = stExecSql(conMPUser,mergesql)
	if nil == Res then
		print("{error merge into }")
		return
	end
	--print ("hello")
	stDoWork()
	curCheckOutData:close()

end
--获得增量数据
function stQueryAddData(yesm,yesa)
	print ("get the add Data")
	local sql  = string.format("select distinct B_CALLER as BCALLER from %s where to_char(B_TIME, \'yyyy-mm-dd hh24:mi:ss\') >= \'%s\' and to_char(B_TIME,\'yyyy-mm-dd hh24:mi:ss\') <= \'%s\' and B_STATUS = 2",tableNmae,yesm,yesa )
	local mergesql = string.format("merge into %s T1 using (%s) T2 on (T1.CC_CALLER = T2.BCALLER) when not matched then insert (CC_CALLER, CC_INDEXFLAG) VALUES(T2.BCALLER, %d)",RecordName,sql,g_indexrecord)
	local Res = stExecSql(conMPUser,mergesql)
	if nil == Res then
		print("{error merge into }")
		return
	end
	--print ("hello")
	stDoWork()
	curCheckOutData:close()
	--插入到数据库进行保存。使用 caller_record
	--sql = string.format("merge into %s a using (select distinct b_caller from %s) b on (a.cc_caller = b.b_caller) when not matched then insert (cc_caller) values(b.b_caller)"

end

function stDoWork()
	--如果打不开文件  失败处理
	local tmpfilename = 'TEMP_'..callerFileName
	local pOutput = io.open(filestorepath .. "\\"..tmpfilename,"w")
	if nil == pOutput then
		C_PrintLog("[ERROR] create output file .." .. callerFileName.." failed")
		return
	end
	--获得数据
	local sql = string.format("select distinct cc_caller as C_CALLER from %s where CC_INDEXFLAG = %d",RecordName,g_indexrecord)
	curCheckOutData = stExecSql(conMPUser,sql)
	if(curCheckOutData == nil) then
		C_PrintLog("[ERROR] get the add data failed")
		return
	end

	local row = curCheckOutData:fetch({},"a")
	local caller = nil
	local areacode = nil
	local line = nil
	while row do
		g_dataCount = g_dataCount + 1
		C_PrintLog("Begin to handle data " ..g_dataCount)
		--print ("Begin to handle data ".. g_dataCount)
		caller = row.C_CALLER or ''
		--print (caller)
		--获取其归属地
		areacode = GetTheAreaId(caller)
		print(areacode)
		--io.open文件 写入
		if judgeDay() == true then
			line = string.format("%s|%s|%s|%d|%s",os.date("%Y%m"),caller, areacode,0,stageinfo)
		else
			line = string.format("%s|%s|%s|%d|%s",os.date("%Y%m"),caller, areacode,1,stageinfo)
		end
		--print(line)

		pOutput:write(line)
		pOutput:write("\r\n")

		row = curCheckOutData:fetch({},"a")
	end
	if pOutput ~= nil then
		--先关闭文件指针，才可以重命名
		pOutput:close()
		os.rename(filestorepath..'\\'..tmpfilename, filestorepath..'\\'..callerFileName)
	end
end

-- 获得文件名
function GetFileName()
	local today = tonumber(os.date("%d"))
	local yt = nil
	if today == 1 then
		yt = string.format("%04d%02d%02d",os.date("%Y"),os.date("%m"),tonumber(os.date("%d")))
	else
		--print("1111")
		yt = string.format("%04s%02s%02d",os.date("%Y"),os.date("%m"),tonumber(os.date("%d"))-1)
	end
	--local yt = string.format("%04s%02s%02d",os.date("%Y"),os.date("%m"),tonumber(os.date("%d"))-1)
	--print(filenameprefix,stagLogo,yt)
	callerFileName = string.format("%s_%s_%s",filenameprefix,stagLogo,yt)
	print (callerFileName)
end
--zhixing
function stExecSql(conn,sql)
	C_PrintLog("exec sql"..sql)
	--print("exec sql " , sql)
	local curRet = conn:execute(sql)
	if nil == curRet then
		C_PrintLog("[error] Exception in db operation")
		--print("[error] Exception indb operation...")
	end
	return curRet
end
--判断是否为一号
function judgeDay()
	local today = tonumber(os.date("%d"))
	if today == 1 then
		return true
	else
		return false
	end
end

function GetTheIndex()
	--zhe这里的字段名需要大写。。。。坑
	local sql = string.format("select bill_fujian.SEQ_CALLER_RECORD.nextval as INDEX_RECORD from dual")
	local Res = stExecSql(conMPUser,sql)
	if nil == Res then
	--	print("[error] get the index failed")
		C_PrintLog("[ERROR]  get the index failed..quit")
		return false
	end
	g_indexrecord = tonumber(Res:fetch({},"a").INDEX_RECORD)
	print ("the index : ".. g_indexrecord)
	Res:close()
	return true
end

function GetTheAreaId(caller)
	local s = string.sub(caller,0,1)
	local area = nil
	if s == "0" then
		area = string.sub(caller,2,4)
	elseif s == "1" then
		local sql = string.format("select citycode from BILL_PROVID where PHONE_PREFIX = substr(\'%s\',1,7)",caller)
		local Ret = stExecSql(conMPUser,sql)
		if nil == Ret then
			area = ''
			return area
		end
		local row = Ret:fetch({},"a")
		if nil == row then
			area = ''
		else
			area = string.sub(row.CITYCODE,2,4)
		end
		Ret:close()
	else
		area  = ''
	end
	return area
end



main()
