--[[集团话单处理脚本]]
---------------------------------------
-- 话单账目项
ACCOUNT_NO = '1002370'
---------------------------------------

-- 需要剔除的主叫号码文件名，内容协商固定为如下全路径
local REMOVE_CALLER_FILE = './removecallers/removecallers.txt'

local g_outputFileName = nil	-- 输出文件名
local g_seqNo = 0               -- 输出文件名中的序列号
local g_removeCallers = {}      -- 需要剔除的主叫列表 key表示主叫 value有值表示需要剔除，为nil表示不需要剔除


--处理一个文件的输出结果
function HandleOneFileOutput(outputFileFullPath, content)
	-- 过滤空文件在此处做
	--[[if content == '' then
		return
	end]]

	pf = io.open(outputFileFullPath, "a")
	if pf == nil then
		print("open output file failed: " .. outputFileFullPath)
		return -1
	end
	pf:write(content)
	pf:close()
	return 0
end

--获取本次数据输出文件名(当输出到一个文件时，每次返回相同的文件名；当输出到各自独立的文件名时，每次返回不同文件名)
-- 参数
--    ext: 扩展参数，以备当多线程情况下，需要在同一时间点使用该方法返回不同文件名时，备用，暂未实际使用
function GetOutputFileName(ext)
	local bOneOutput = true
	if true == bOneOutput then
		----[[-- 所有结果输出到一个文件时使用如下策略: 输出文件名只计算一次，以后每次均返回该文件名
		if g_outputFileName == nil then
			-- 根据各省要求产生文件名

			-- 贵州固网格式
			g_outputFileName = "group_" .. os.date("%Y%m%d%H%M%S", os.time()) .. ".txt"

		end
		--]]
	else
		----[[ -- 当每次需要返回不同文件名时，可使用类似下述方法
		g_outputFileName = "group_" .. os.date("%Y%m%d%H%M%S", os.time()) .. string.format("%03d", g_seqNo) .. ".txt"
		g_seqNo = g_seqNo + 1
		if g_seqNo > 999 then g_seqNo = 0 end
		--]]
	end

	print(g_outputFileName)

	return g_outputFileName
end

-- 处理一行数据输
--[[
参数:
	srcline: 行数据
返回:
	int 结果 0成功 非0失败
	outputline 输出结果
]]
function HandleLine(srcline)
	-- 贵州固网要求: 主叫 被叫 开始时间 时长 账目项(1002370) 费用(元，2位小数点)
	local caller, called, starttime, timelen, fee = string.match(srcline, "(%d+).-,(%d+).-,(%d+).-,.-,(%d+).-,.-,(.-)%s*,")
	if caller == nil then -- 解析失败
		return -1, ''
	end

	-- 贵州固网特殊需求，过滤主叫(需要过滤的主叫提供在一个txt文件中，每行一个主叫号码的形式)
	if g_removeCallers[caller] then
		return 0, ""
	end

	return 0, string.format("%s,%s,%s,%s,%s,%s\n", caller, called, starttime, timelen, ACCOUNT_NO, fee)
end

--ret, s = HandleLine("08863286155         , 16895689            ,20151204120504,20151204120601,57      ,0.80    ,0.80    ,16895689")
--print(ret, s)

-- 加载剔除号码文件
function LoadRemoveCaller()
	local pf = io.open(REMOVE_CALLER_FILE, "r")
	if pf == nil then
		print("load remove caller file failed")
		return
	end

	for line in pf:lines(REMOVE_CALLER_FILE) do
		g_removeCallers[line] = 1
	end
	pf:close()

	--[[for j,k in pairs(g_removeCallers) do
		print(j, k)
	end]]
end

LoadRemoveCaller()

