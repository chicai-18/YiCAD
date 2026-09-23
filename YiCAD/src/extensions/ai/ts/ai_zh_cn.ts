<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>AIExtension</name>
    <message>
        <location filename="../AIExtension.cpp"/>
        <source>AI Assistant</source>
        <translation>AI 助手</translation>
    </message>
    <message>
        <location filename="../AIExtension.cpp"/>
        <source>AI Settings</source>
        <translation>AI 设置</translation>
    </message>
</context>
<context>
    <name>AIDialog</name>
    <message>
        <location filename="../ui/AIDialog.cpp" line="165"/>
        <source>AI is processing, please wait...</source>
        <translation>AI 处理中，请稍候...</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="31"/>
        <source>AI Assistant</source>
        <translation>AI 助手</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="75"/>
        <source>Mode:</source>
        <translation>模式：</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="77"/>
        <source>Q&amp;A</source>
        <translation>问答</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="78"/>
        <source>Modeling</source>
        <translation>建模</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="79"/>
        <source>Auto</source>
        <translation>自动</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="89"/>
        <source>AI conversation will be displayed here...</source>
        <translation>AI 对话将显示在这里...</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="94"/>
        <source>Input</source>
        <translation>输入</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="100"/>
        <source>Enter your question or modeling command...</source>
        <translation>输入您的问题或建模命令...</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="105"/>
        <source>Send</source>
        <translation>发送</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="115"/>
        <source>Execute</source>
        <translation>执行</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="116"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="117"/>
        <source>Config</source>
        <translation>配置</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="118"/>
        <source>History</source>
        <translation>历史</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="88"/>
        <source>New Session</source>
        <translation>新会话</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="159"/>
        <source>User</source>
        <translation>用户</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="206"/>
        <source>System</source>
        <translation>系统</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="206"/>
        <source>Executing: %1</source>
        <translation>正在执行：%1</translation>
    </message>
</context>
<context>
    <name>AIIntentRouter</name>
    <message>
        <location filename="../AIIntentRouter.cpp" line="294"/>
        <source>Input is empty.</source>
        <translation>输入为空</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="295"/>
        <source>Please enter your question or modeling command.</source>
        <translation>请输入您的问题或建模指令</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="307"/>
        <source>Manually set to QA mode.</source>
        <translation>手动指定为问答模式</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="308"/>
        <source>Will enter QA pipeline (RAG).</source>
        <translation>将进入问答链路（RAG）</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="318"/>
        <source>Manually set to Modeling mode.</source>
        <translation>手动指定为建模模式</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="319"/>
        <source>Will enter modeling execution pipeline.</source>
        <translation>将进入建模执行链路</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="413"/>
        <source>No matching QA or modeling keywords found; unable to determine intent (QA_score=%1, Modeling_score=%2)</source>
        <translation>未匹配到明确的问答或建模关键词，无法确定意图 (QA得分=%1, 建模得分=%2)</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="416"/>
        <source>Please provide more details: are you looking for information about a feature, or do you want to perform a modeling operation?</source>
        <translation>请补充更多信息：您是想了解某个功能，还是执行建模操作？</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="429"/>
        <source>Both QA and modeling keywords matched; classified as mixed intent (QA_score=%1, Modeling_score=%2)</source>
        <translation>同时匹配到问答关键词和建模关键词，判定为混合意图 (QA得分=%1, 建模得分=%2)</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="432"/>
        <source>Provide instructions first, then ask whether to execute the modeling operation.</source>
        <translation>先提供操作说明，然后询问是否需要直接执行建模操作</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="445"/>
        <source>QA keywords matched; classified as QA intent (QA_score=%1, Modeling_score=%2)</source>
        <translation>匹配到问答关键词，判定为问答意图 (QA得分=%1, 建模得分=%2)</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="460"/>
        <source>Modeling keywords matched; classified as modeling intent (QA_score=%1, Modeling_score=%2)</source>
        <translation>匹配到建模关键词，判定为建模意图 (QA得分=%1, 建模得分=%2)</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="475"/>
        <source>QA score is higher (QA=%1 &gt; Modeling=%2); classified as QA.</source>
        <translation>问答倾向得分更高 (QA=%1 > 建模=%2)，判定为问答意图</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="485"/>
        <source>Modeling score is higher (Modeling=%1 &gt; QA=%2); classified as modeling.</source>
        <translation>建模倾向得分更高 (建模=%1 > QA=%2)，判定为建模意图</translation>
    </message>
</context>
<context>
    <name>AIPipeline</name>
    <message>
        <location filename="../AIPipeline.cpp" line="425"/>
        <source>The following parameters are required: %1
Please provide their values directly in the dialog.</source>
        <translation>需要以下参数：%1
请直接在对话中提供这些参数的值。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="552"/>
        <source>Drawing failed: %1</source>
        <translation>绘图失败：%1</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="134"/>
        <source>AI features require an API key. Please configure it in AI Settings.</source>
        <translation>AI 功能需要 API Key，请在 AI 设置中配置。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="216"/>
        <source>Unable to parse your command.</source>
        <translation>无法解析你的指令</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="195"/>
        <source>Hello! I&apos;m YiCAD AI Assistant. I can help you with:
• Q&amp;A: Ask me anything about YiCAD features and usage
• Modeling: Describe what you want to draw or modify, and I&apos;ll execute it on the canvas

Switch modes via the dropdown menu (Q&amp;A / Modeling / Auto), or just type your request in Auto mode.</source>
        <translation>你好！我是 YiCAD AI 助手。我可以帮你：
• 问答：询问 YiCAD 功能和使用方法
• 建模：描述你想绘制或修改的内容，我会在画布上执行

通过下拉菜单切换模式（问答 / 建模 / 自动），或直接在自动模式下输入你的需求。</translation>
    </message>
</context>
<context>
    <name>ContextResolver</name>
    <message>
        <location filename="../ContextResolver.cpp" line="94"/>
        <source>[%1] %2 (resolved via %3)</source>
        <translation>[%1] %2 (通过 %3 解析)</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="163"/>
        <source>No selection required (None mode).</source>
        <translation>无需选择（None 模式）。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="181"/>
        <source>ContextResolver: unknown SelectionMode.</source>
        <translation>ContextResolver: 未知的 SelectionMode。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="207"/>
        <source>ContextResolver: no selected entities matching type hint '%1'.</source>
        <translation>ContextResolver: 没有匹配类型提示 '%1' 的已选实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="211"/>
        <source>ContextResolver: no entities currently selected.</source>
        <translation>ContextResolver: 当前没有选中的实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="221"/>
        <source>Resolved %1 entity/entities from current selection (filtered by type: %2).</source>
        <translation>从当前选择中解析了 %1 个实体（按类型过滤: %2）。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="226"/>
        <source>Resolved %1 entity/entities from current selection.</source>
        <translation>从当前选择中解析了 %1 个实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="281"/>
        <source> (filtered by type: %1)</source>
        <translation>（按类型过滤: %1）</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="283"/>
        <source>Resolved %1 entity/entities from previous turn (user said: "%2")%3.</source>
        <translation>从上一轮对话中解析了 %1 个实体（用户说: "%2"）%3。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="324"/>
        <source>Resolved 1 entity as last created (fallback: no history available).</source>
        <translation>将 1 个实体解析为最后创建的（回退: 无历史记录可用）。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="335"/>
        <source>ContextResolver: no last-created entity found (type hint: %1). Create an entity first, then reference it in the next turn.</source>
        <translation>ContextResolver: 未找到最后创建的实体（类型提示: %1）。请先创建一个实体，然后在下一轮中引用它。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="359"/>
        <source>ContextResolver: no visible entities in document.</source>
        <translation>ContextResolver: 文档中没有可见实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="366"/>
        <source>Resolved %1 entity/entities (all visible).</source>
        <translation>解析了 %1 个实体（所有可见）。</translation>
    </message>
</context>
<context>
    <name>DeepSeekProvider</name>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="256"/>
        <source>(no message)</source>
        <translation>（无消息）</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="263"/>
        <source> [%1]</source>
        <translation> [%1]</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="267"/>
        <source> code=%1</source>
        <translation> code=%1</translation>
    </message>
</context>
<context>
    <name>DirectEntityExecutor</name>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="72"/>
        <source>DirectEntityExecutor: unsupported intent '%1'. Only draw_point / draw_line / draw_circle / draw_rectangle / draw_ellipse are supported.</source>
        <translation>DirectEntityExecutor: 不支持的意图 '%1'。仅支持 draw_point / draw_line / draw_circle / draw_rectangle / draw_ellipse。</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="93"/>
        <source>DirectEntityExecutor::draw_point -- %1</source>
        <translation>DirectEntityExecutor::draw_point -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="99"/>
        <source>Create Point</source>
        <translation>创建点</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="111"/>
        <source>DirectEntityExecutor::draw_line -- %1</source>
        <translation>DirectEntityExecutor::draw_line -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="122"/>
        <source>Create Line</source>
        <translation>创建线</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="136"/>
        <source>DirectEntityExecutor::draw_circle -- %1</source>
        <translation>DirectEntityExecutor::draw_circle -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="147"/>
        <source>Create Circle</source>
        <translation>创建圆</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="159"/>
        <source>DirectEntityExecutor::draw_rectangle -- %1</source>
        <translation>DirectEntityExecutor::draw_rectangle -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="184"/>
        <source>Create Rectangle</source>
        <translation>创建矩形</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="200"/>
        <source>DirectEntityExecutor::draw_ellipse -- %1</source>
        <translation>DirectEntityExecutor::draw_ellipse -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="235"/>
        <source>Create Ellipse</source>
        <translation>创建椭圆</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="249"/>
        <source>missing required param '%1'</source>
        <translation>缺少必需参数 '%1'</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="255"/>
        <source>param '%1' must be a JSON array [x, y]</source>
        <translation>参数 '%1' 必须是 JSON 数组 [x, y]</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="261"/>
        <source>param '%1' array must have at least 2 elements (x, y)</source>
        <translation>参数 '%1' 数组必须至少有 2 个元素 (x, y)</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="268"/>
        <source>param '%1'[0] (x) is not a number</source>
        <translation>参数 '%1'[0] (x) 不是数字</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="272"/>
        <source>param '%1'[1] (y) is not a number</source>
        <translation>参数 '%1'[1] (y) 不是数字</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="289"/>
        <source>param '%1' must be a number</source>
        <translation>参数 '%1' 必须是数字</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="316"/>
        <source>DirectEntityExecutor: internal error — null document or entity.</source>
        <translation>DirectEntityExecutor: 内部错误 — 文档或实体为空。</translation>
    </message>
</context>
<context>
    <name>LLMCommandBridge</name>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="166"/>
        <source>LLMCommandBridge: unable to extract JSON from LLM response. The model may have returned text without a valid JSON block.</source>
        <translation>LLMCommandBridge: 无法从 LLM 响应中提取 JSON。模型可能返回了不含有效 JSON 块的文本。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="179"/>
        <source>LLMCommandBridge: JSON parse failed -- </source>
        <translation>LLMCommandBridge: JSON 解析失败 -- </translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="244"/>
        <source>JSON root is not an object (expected {...}), got an array or scalar.</source>
        <translation>JSON 根不是对象（期望 {...}），得到的是数组或标量。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="267"/>
        <source>LLMCommandBridge: missing required field 'intent' in command JSON.</source>
        <translation>LLMCommandBridge: 命令 JSON 中缺少必需字段 'intent'。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="275"/>
        <source>LLMCommandBridge: field 'intent' must be a string.</source>
        <translation>LLMCommandBridge: 字段 'intent' 必须是字符串。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="286"/>
        <source>Unknown intent: %1. The command may not be executable.</source>
        <translation>未知意图: %1。该命令可能无法执行。</translation>
    </message>
</context>
<context>
    <name>LLMSettingsPage</name>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="42"/>
        <source>LLM Settings</source>
        <translation>LLM 设置</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="62"/>
        <source>Configure AI language model connection parameters.
AI features are unavailable without an API Key.</source>
        <translation>配置 AI 语言模型连接参数。
没有 API 密钥将无法使用 AI 功能。</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="71"/>
        <source>deepseek</source>
        <translation>deepseek</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="72"/>
        <source>Provider:</source>
        <translation>提供商：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="76"/>
        <source>Base URL:</source>
        <translation>基础 URL：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="80"/>
        <source>Model:</source>
        <translation>模型：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="84"/>
        <source> sec</source>
        <translation> 秒</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="85"/>
        <source>Request timeout in seconds</source>
        <translation>请求超时（秒）</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="86"/>
        <source>Timeout:</source>
        <translation>超时：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="92"/>
        <source>Generation randomness: 0=deterministic, 2=maximum randomness</source>
        <translation>生成随机性：0=确定性，2=最大随机性</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="93"/>
        <source>Temperature:</source>
        <translation>温度：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="97"/>
        <source>sk-...</source>
        <translation>sk-...</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="98"/>
        <source>API Key is stored encrypted and will not be saved in plain text</source>
        <translation>API 密钥已加密存储，不会以明文保存</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="99"/>
        <source>API Key:</source>
        <translation>API 密钥：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="104"/>
        <source>Note: API Key uses XOR obfuscation (temporary).
Will be replaced with system-level secure storage (DPAPI/Keychain) in release.</source>
        <translation>注意：API 密钥目前使用 XOR 混淆（临时方案）。
发布版本中将替换为系统级安全存储（DPAPI/Keychain）。</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="113"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="114"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="145"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="146"/>
        <source>LLM settings service is not initialized.</source>
        <translation>LLM 设置服务未初始化。</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="136"/>
        <source>(Set; enter new key to overwrite)</source>
        <translation>（已设置；输入新密钥以覆盖）</translation>
    </message>
</context>
</TS>
