<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>AIDialog</name>
    <message>
        <location filename="../ui/AIDialog.cpp" line="181"/>
        <source>AI is processing, please wait...</source>
        <translation>AI 处理中，请稍候...</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="33"/>
        <source>AI Assistant</source>
        <translation>AI 助手</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="77"/>
        <source>Mode:</source>
        <translation>模式：</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="79"/>
        <source>Q&amp;A</source>
        <translation>问答</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="80"/>
        <source>Modeling</source>
        <translation>建模</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="81"/>
        <source>Auto</source>
        <translation>自动</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="121"/>
        <source>AI conversation will be displayed here...</source>
        <translation>AI 对话将显示在这里...</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="126"/>
        <source>Input</source>
        <translation>输入</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="132"/>
        <source>Enter your question or modeling command...</source>
        <translation>输入您的问题或建模命令...</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="137"/>
        <source>Send</source>
        <translation>发送</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="113"/>
        <source>Config</source>
        <translation>配置</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="89"/>
        <source>New Session</source>
        <translation>新会话</translation>
    </message>
    <message>
        <location filename="../ui/AIDialog.cpp" line="192"/>
        <location filename="../ui/AIDialog.cpp" line="220"/>
        <source>User</source>
        <translation>用户</translation>
    </message>
</context>
<context>
    <name>AIExtension</name>
    <message>
        <location filename="../AIExtension.cpp" line="57"/>
        <source>AI Assistant</source>
        <translation>AI 助手</translation>
    </message>
    <message>
        <location filename="../AIExtension.cpp" line="71"/>
        <source>AI Settings</source>
        <translation>AI 设置</translation>
    </message>
</context>
<context>
    <name>AIIntentRouter</name>
    <message>
        <location filename="../AIIntentRouter.cpp" line="48"/>
        <source>Input is empty.</source>
        <translation>输入为空</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="49"/>
        <source>Please enter your question or modeling command.</source>
        <translation>请输入您的问题或建模指令</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="59"/>
        <source>Manually set to QA mode.</source>
        <translation>手动指定为问答模式</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="60"/>
        <source>Will enter QA pipeline (RAG).</source>
        <translation>将进入问答链路（RAG）</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="69"/>
        <source>Manually set to Modeling mode.</source>
        <translation>手动指定为建模模式</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="70"/>
        <source>Will enter modeling execution pipeline.</source>
        <translation>将进入建模执行链路</translation>
    </message>
    <message>
        <location filename="../AIIntentRouter.cpp" line="79"/>
        <source>Awaiting LLM classification...</source>
        <translation>等待 LLM 分类...</translation>
    </message>
</context>
<context>
    <name>AILLMClassifier</name>
    <message>
        <location filename="../AILLMClassifier.cpp" line="86"/>
        <source>LLM API is not available (API Key not configured).</source>
        <translation>LLM 接口不可用（未配置 API 密钥）。</translation>
    </message>
    <message>
        <location filename="../AILLMClassifier.cpp" line="119"/>
        <source>Failed to parse LLM classification response: %1</source>
        <translation>无法解析 LLM 分类响应：%1</translation>
    </message>
    <message>
        <location filename="../AILLMClassifier.cpp" line="146"/>
        <source>LLM classification timed out after 3 seconds.</source>
        <translation>LLM 分类超时（3 秒）。</translation>
    </message>
</context>
<context>
    <name>AIPipeline</name>
    <message>
        <location filename="../AIPipeline.cpp" line="340"/>
        <location filename="../AIPipeline.cpp" line="403"/>
        <source>The following parameters are required: %1
Please provide their values directly in the dialog.</source>
        <translation>需要以下参数：%1
请直接在对话中提供这些参数的值。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="384"/>
        <source>Parse Error</source>
        <translation>解析错误</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="385"/>
        <source>Failed to parse modeling command from LLM response:
%1

Raw response:
%2</source>
        <translation>无法从 LLM 响应中解析建模命令：
%1

原始响应：
%2</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="424"/>
        <source>Modeling Error</source>
        <translation>建模错误</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="435"/>
        <source>Context Error</source>
        <translation>上下文错误</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="436"/>
        <source>Failed to resolve selection: %1</source>
        <translation>无法解析选择：%1</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="559"/>
        <location filename="../AIPipeline.cpp" line="588"/>
        <source>Drawing executor is not available (no document open).</source>
        <translation>绘图执行器不可用（没有打开的文档）。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="565"/>
        <source>Drawing failed: %1</source>
        <translation>绘图失败：%1</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="146"/>
        <source>AI features require an API key. Please configure it in AI Settings.</source>
        <translation>AI 功能需要 API Key，请在 AI 设置中配置。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="59"/>
        <location filename="../AIPipeline.cpp" line="72"/>
        <location filename="../AIPipeline.cpp" line="82"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="60"/>
        <source>LLM classifier is not available (API Key not configured). Auto mode will be limited.</source>
        <translation>LLM 分类器不可用（未配置 API 密钥），自动模式的功能将受限。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="73"/>
        <source>RAG knowledge base is empty. QA functionality will be limited. Check docs/ directory.</source>
        <translation>RAG 知识库为空，问答功能将受限。请检查 docs/ 目录。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="83"/>
        <source>No document open. Modeling commands are unavailable.</source>
        <translation>没有打开的文档，建模命令不可用。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="122"/>
        <location filename="../AIPipeline.cpp" line="252"/>
        <location filename="../AIPipeline.cpp" line="267"/>
        <location filename="../AIPipeline.cpp" line="292"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="122"/>
        <source>Input is empty.</source>
        <translation>输入为空</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="145"/>
        <location filename="../AIPipeline.cpp" line="203"/>
        <location filename="../AIPipeline.cpp" line="223"/>
        <location filename="../AIPipeline.cpp" line="276"/>
        <location filename="../AIPipeline.cpp" line="348"/>
        <location filename="../AIPipeline.cpp" line="354"/>
        <location filename="../AIPipeline.cpp" line="379"/>
        <location filename="../AIPipeline.cpp" line="411"/>
        <location filename="../AIPipeline.cpp" line="447"/>
        <source>AI</source>
        <translation>AI</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="150"/>
        <location filename="../AIPipeline.cpp" line="195"/>
        <location filename="../AIPipeline.cpp" line="258"/>
        <location filename="../AIPipeline.cpp" line="298"/>
        <location filename="../AIPipeline.cpp" line="332"/>
        <location filename="../AIPipeline.cpp" line="395"/>
        <location filename="../AIPipeline.cpp" line="441"/>
        <source>System</source>
        <translation>系统</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="150"/>
        <source>Analyzing intent...</source>
        <translation>正在分析意图...</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="196"/>
        <source>Detected mixed intent. Providing help answer first.</source>
        <translation>检测到混合意图，先提供帮助解答。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="224"/>
        <source>Unable to parse your command.</source>
        <translation>无法解析你的指令</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="253"/>
        <source>RAG pipeline is not initialized. QA mode is unavailable.</source>
        <translation>RAG 链路未初始化，问答模式不可用。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="259"/>
        <source>Intent: Q&amp;A (confidence: %1%). Searching knowledge base...</source>
        <translation>意图：问答（置信度：%1%）。正在检索知识库...</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="281"/>
        <source>RAG Error</source>
        <translation>RAG 错误</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="293"/>
        <source>Modeling mode requires an open document. Please open or create a drawing first.</source>
        <translation>建模模式需要打开的文档，请先打开或新建图纸。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="299"/>
        <source>Intent: Modeling (%1%). Generating command...</source>
        <translation>意图：建模（%1%）。正在生成命令...</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="322"/>
        <source>Stream Parse</source>
        <translation>流式解析</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="323"/>
        <source>Failed to parse streaming command: %1
JSON: %2</source>
        <translation>无法解析流式命令：%1
JSON：%2</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="333"/>
        <location filename="../AIPipeline.cpp" line="396"/>
        <source>⚠ High-risk operation: %1</source>
        <translation>⚠ 高风险操作：%1</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="569"/>
        <source>Drawing completed: %1</source>
        <translation>绘图完成：%1</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="573"/>
        <location filename="../AIPipeline.cpp" line="601"/>
        <source>Created %1 entity(s):</source>
        <translation>已创建 %1 个实体：</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="594"/>
        <source>Compound drawing failed: %1</source>
        <translation>组合绘图失败：%1</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="597"/>
        <source>Compound drawing completed: %1</source>
        <translation>组合绘图完成：%1</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="614"/>
        <source>Command intent &apos;%1&apos; is not yet supported by the executors.</source>
        <translation>执行器尚不支持命令意图 &apos;%1&apos;。</translation>
    </message>
    <message>
        <location filename="../AIPipeline.cpp" line="204"/>
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
        <location filename="../ContextResolver.cpp" line="165"/>
        <source>No selection required (None mode).</source>
        <translation>无需选择（None 模式）。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="174"/>
        <source>ContextResolver: unknown SelectionMode.</source>
        <translation>ContextResolver: 未知的 SelectionMode。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="200"/>
        <source>ContextResolver: no selected entities matching type hint &apos;%1&apos;.</source>
        <translation>ContextResolver: 没有匹配类型提示 &apos;%1&apos; 的已选实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="204"/>
        <source>ContextResolver: no entities currently selected.</source>
        <translation>ContextResolver: 当前没有选中的实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="214"/>
        <source>Resolved %1 entity/entities from current selection (filtered by type: %2).</source>
        <translation>从当前选择中解析了 %1 个实体（按类型过滤: %2）。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="219"/>
        <source>Resolved %1 entity/entities from current selection.</source>
        <translation>从当前选择中解析了 %1 个实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="274"/>
        <source> (filtered by type: %1)</source>
        <translation>（按类型过滤: %1）</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="276"/>
        <source>Resolved %1 entity/entities from previous turn (user said: &quot;%2&quot;)%3.</source>
        <translation>从上一轮对话中解析了 %1 个实体（用户说: &quot;%2&quot;）%3。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="317"/>
        <source>Resolved 1 entity as last created (fallback: no history available).</source>
        <translation>将 1 个实体解析为最后创建的（回退: 无历史记录可用）。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="328"/>
        <source>ContextResolver: no last-created entity found (type hint: %1). Create an entity first, then reference it in the next turn.</source>
        <translation>ContextResolver: 未找到最后创建的实体（类型提示: %1）。请先创建一个实体，然后在下一轮中引用它。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="352"/>
        <source>ContextResolver: no visible entities in document.</source>
        <translation>ContextResolver: 文档中没有可见实体。</translation>
    </message>
    <message>
        <location filename="../ContextResolver.cpp" line="359"/>
        <source>Resolved %1 entity/entities (all visible).</source>
        <translation>解析了 %1 个实体（所有可见）。</translation>
    </message>
</context>
<context>
    <name>DeepSeekProvider</name>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="78"/>
        <source>LLM configuration service is not initialized.</source>
        <translation>LLM 配置服务未初始化。</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="85"/>
        <source>API Key not configured. Please set it in AI settings.</source>
        <translation>未配置 API 密钥，请在 AI 设置中设置。</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="192"/>
        <location filename="../DeepSeekProvider.cpp" line="197"/>
        <source>

[⚠ Request cancelled before completion]</source>
        <translation>

[⚠ 请求在完成前已取消]</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="203"/>
        <source>Request timed out (no data received within the timeout period). Please try simplifying your request or increasing the timeout setting.</source>
        <translation>请求超时（超时时间内未收到数据）。请尝试简化请求，或增大超时设置。</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="217"/>
        <source>DeepSeek API error (HTTP %1): %2</source>
        <translation>DeepSeek API 错误（HTTP %1）：%2</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="225"/>
        <source>HTTP %1: %2</source>
        <translation>HTTP %1：%2</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="233"/>
        <source>Network request failed: %1</source>
        <translation>网络请求失败：%1</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="411"/>
        <source>(no message)</source>
        <translation>（无消息）</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="418"/>
        <source> [%1]</source>
        <translation> [%1]</translation>
    </message>
    <message>
        <location filename="../DeepSeekProvider.cpp" line="422"/>
        <source> code=%1</source>
        <translation> code=%1</translation>
    </message>
</context>
<context>
    <name>DirectEntityExecutor</name>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="83"/>
        <source>DirectEntityExecutor: unsupported intent &apos;%1&apos;. Only draw_point / draw_line / draw_circle / draw_rectangle / draw_ellipse / draw_arc are supported.</source>
        <translation>DirectEntityExecutor: 不支持的意图 &apos;%1&apos;。仅支持 draw_point / draw_line / draw_circle / draw_rectangle / draw_ellipse / draw_arc。</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="102"/>
        <source>DirectEntityExecutor::draw_compound: steps array is empty.</source>
        <translation>DirectEntityExecutor::draw_compound: steps 数组为空。</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="108"/>
        <source>Create Compound</source>
        <translation>创建组合图形</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="145"/>
        <source>draw_compound step %1: unsupported intent &apos;%2&apos;</source>
        <translation>draw_compound 第 %1 步：不支持的意图 &apos;%2&apos;</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="154"/>
        <source>draw_compound step %1: failed to create entity -- %2</source>
        <translation>draw_compound 第 %1 步：创建实体失败 -- %2</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="187"/>
        <source>DirectEntityExecutor::draw_point -- %1</source>
        <translation>DirectEntityExecutor::draw_point -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="191"/>
        <source>Create Point</source>
        <translation>创建点</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="203"/>
        <source>DirectEntityExecutor::draw_line -- %1</source>
        <translation>DirectEntityExecutor::draw_line -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="207"/>
        <source>Create Line</source>
        <translation>创建线</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="219"/>
        <source>DirectEntityExecutor::draw_circle -- %1</source>
        <translation>DirectEntityExecutor::draw_circle -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="223"/>
        <source>Create Circle</source>
        <translation>创建圆</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="235"/>
        <source>DirectEntityExecutor::draw_rectangle -- %1</source>
        <translation>DirectEntityExecutor::draw_rectangle -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="239"/>
        <source>Create Rectangle</source>
        <translation>创建矩形</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="251"/>
        <source>DirectEntityExecutor::draw_ellipse -- %1</source>
        <translation>DirectEntityExecutor::draw_ellipse -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="255"/>
        <source>Create Ellipse</source>
        <translation>创建椭圆</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="267"/>
        <source>DirectEntityExecutor::draw_arc -- %1</source>
        <translation>DirectEntityExecutor::draw_arc -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="271"/>
        <source>Create Arc</source>
        <translation>创建圆弧</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="283"/>
        <source>DirectEntityExecutor::draw_text -- %1</source>
        <translation>DirectEntityExecutor::draw_text -- %1</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="287"/>
        <source>Create Text</source>
        <translation>创建文字</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="428"/>
        <source>missing required param &apos;text&apos;</source>
        <translation>缺少必需参数 &apos;text&apos;</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="433"/>
        <source>param &apos;text&apos; must be a non-empty string</source>
        <translation>参数 &apos;text&apos; 必须是非空字符串</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="514"/>
        <location filename="../DirectEntityExecutor.cpp" line="554"/>
        <source>missing required param &apos;%1&apos;</source>
        <translation>缺少必需参数 &apos;%1&apos;</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="520"/>
        <source>param &apos;%1&apos; must be a JSON array [x, y]</source>
        <translation>参数 &apos;%1&apos; 必须是 JSON 数组 [x, y]</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="526"/>
        <source>param &apos;%1&apos; array must have at least 2 elements (x, y)</source>
        <translation>参数 &apos;%1&apos; 数组必须至少有 2 个元素 (x, y)</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="533"/>
        <source>param &apos;%1&apos;[0] (x) is not a number</source>
        <translation>参数 &apos;%1&apos;[0] (x) 不是数字</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="537"/>
        <source>param &apos;%1&apos;[1] (y) is not a number</source>
        <translation>参数 &apos;%1&apos;[1] (y) 不是数字</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="560"/>
        <source>param &apos;%1&apos; must be a number</source>
        <translation>参数 &apos;%1&apos; 必须是数字</translation>
    </message>
    <message>
        <location filename="../DirectEntityExecutor.cpp" line="581"/>
        <source>DirectEntityExecutor: internal error — null document or entity.</source>
        <translation>DirectEntityExecutor: 内部错误 — 文档或实体为空。</translation>
    </message>
</context>
<context>
    <name>LLMCommandBridge</name>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="150"/>
        <source>LLMCommandBridge: unable to extract JSON from LLM response. The model may have returned text without a valid JSON block.</source>
        <translation>LLMCommandBridge: 无法从 LLM 响应中提取 JSON。模型可能返回了不含有效 JSON 块的文本。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="163"/>
        <source>LLMCommandBridge: JSON parse failed -- </source>
        <translation>LLMCommandBridge: JSON 解析失败 -- </translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="228"/>
        <source>JSON root is not an object (expected {...}), got an array or scalar.</source>
        <translation>JSON 根不是对象（期望 {...}），得到的是数组或标量。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="251"/>
        <source>LLMCommandBridge: missing required field &apos;intent&apos; in command JSON.</source>
        <translation>LLMCommandBridge: 命令 JSON 中缺少必需字段 &apos;intent&apos;。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="259"/>
        <source>LLMCommandBridge: field &apos;intent&apos; must be a string.</source>
        <translation>LLMCommandBridge: 字段 &apos;intent&apos; 必须是字符串。</translation>
    </message>
    <message>
        <location filename="../LLMCommandBridge.cpp" line="270"/>
        <source>Unknown intent: %1. The command may not be executable.</source>
        <translation>未知意图: %1。该命令可能无法执行。</translation>
    </message>
</context>
<context>
    <name>LLMSettingsPage</name>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="45"/>
        <source>LLM Settings</source>
        <translation>LLM 设置</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="65"/>
        <source>Configure AI language model connection parameters.
AI features are unavailable without an API Key.</source>
        <translation>配置 AI 语言模型连接参数。
没有 API 密钥将无法使用 AI 功能。</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="74"/>
        <source>deepseek</source>
        <translation>deepseek</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="75"/>
        <source>Provider:</source>
        <translation>提供商：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="79"/>
        <source>Base URL:</source>
        <translation>基础 URL：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="83"/>
        <source>Model:</source>
        <translation>模型：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="87"/>
        <source> sec</source>
        <translation> 秒</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="88"/>
        <source>Request timeout in seconds</source>
        <translation>请求超时（秒）</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="89"/>
        <source>Timeout:</source>
        <translation>超时：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="95"/>
        <source>Generation randomness: 0=deterministic, 2=maximum randomness</source>
        <translation>生成随机性：0=确定性，2=最大随机性</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="96"/>
        <source>Temperature:</source>
        <translation>温度：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="101"/>
        <location filename="../ui/LLMSettingsPage.cpp" line="193"/>
        <source>sk-...</source>
        <translation>sk-...</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="102"/>
        <source>API Key is stored encrypted and will not be saved in plain text</source>
        <translation>API 密钥已加密存储，不会以明文保存</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="111"/>
        <source>API Key:</source>
        <translation>API 密钥：</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="121"/>
        <source>Clear saved API Key</source>
        <translation>清除已保存的 API 密钥</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="132"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="133"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="164"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="165"/>
        <source>LLM settings service is not initialized.</source>
        <translation>LLM 设置服务未初始化。</translation>
    </message>
    <message>
        <location filename="../ui/LLMSettingsPage.cpp" line="155"/>
        <source>(Set; enter new key to overwrite)</source>
        <translation>（已设置；输入新密钥以覆盖）</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../ContextResolver.cpp" line="95"/>
        <source>[%1] %2 (resolved via %3)</source>
        <translation>[%1] %2 (通过 %3 解析)</translation>
    </message>
</context>
<context>
    <name>RAGPipeline</name>
    <message>
        <location filename="../RAGPipeline.cpp" line="106"/>
        <source>RAG pipeline is not initialized. Please load knowledge sources first.</source>
        <translation>RAG 链路未初始化，请先加载知识源。</translation>
    </message>
    <message>
        <location filename="../RAGPipeline.cpp" line="113"/>
        <source>Question is empty.</source>
        <translation>问题为空。</translation>
    </message>
    <message>
        <location filename="../RAGPipeline.cpp" line="343"/>
        <source>(Empty response from model)</source>
        <translation>（模型返回了空响应）</translation>
    </message>
</context>
</TS>
