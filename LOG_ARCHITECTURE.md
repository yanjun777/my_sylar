# Sylar 日志模块设计说明

## 1. 这个日志库在做什么

一句话：把业务代码产生的日志事件，按指定格式，输出到不同目标（终端、文件等）。

核心目标有三个：

- 业务侧调用简单（只关心“记录什么”）
- 输出侧可扩展（想写到哪里就加一个新 Appender）
- 格式可配置（同一事件可按不同 pattern 输出）

---

## 2. 各模块职责（对应 `log.h`）

### `LogEvent`：日志“数据包”

它只负责承载一次日志记录的上下文数据：

- 文件名、行号
- 时间、线程信息
- 日志正文 `content`

可以把它理解成“日志请求对象”。

### `LogLevel`：日志等级定义

它定义了 `DEBUG/INFO/WARN/ERROR/FATAL`，并提供 `toString()` 做字符串映射。  
作用：统一比较和展示规则，避免到处写魔法数字/字符串。

### `LogFormatter`：格式引擎

它负责把 `LogEvent` 转成最终字符串，核心成员：

- `m_pattern`：格式模板（例如 `%p %m%n`）
- `m_items`：解析后得到的“格式项对象列表”

`init()` 的职责是把 pattern 解析为 token，再构造成 `FormatItem` 子类对象；`format()` 负责顺序执行这些对象，拼出日志文本。

### `FormatItem`（抽象基类）+ 各子类：可插拔格式项

`FormatItem` 定义统一接口：

- `format(std::ostream&, logger, level, event)`

子类如 `MessageFormatItem`、`LevelFormatItem`、`ElapseFormatItem` 各自只处理一个字段。  
这是一种典型“策略对象”设计。

### `LogAppender`：输出目标抽象层

定义统一输出接口：

- `log(logger, level, event)`

并持有自己的：

- `m_level`（本 appender 的过滤等级）
- `m_formatter`（本 appender 的格式器）

### `StdioLogAppender` / `FileLogAppender`：输出实现层

- `StdioLogAppender`：输出到标准输出
- `FileLogAppender`：输出到文件，带 `reopen()`

它们都依赖 `LogFormatter` 生成字符串，但互不关心对方实现。

### `Logger`：调度与聚合中心

`Logger` 管理多个 appender，负责：

- 接收日志请求
- 按 logger 级别过滤
- 把事件分发给所有 appender

可理解为“日志总线 + 路由器”。

---

## 3. 它如何降低耦合（重点）

### 3.1 把“记录什么”和“输出到哪”分开

业务代码构造 `LogEvent`，不关心最终去终端还是文件。  
`Logger` 统一调度，`Appender` 决定写到哪里。

### 3.2 把“输出到哪”和“怎么格式化”分开

`Appender` 不直接拼字符串，交给 `Formatter`。  
因此可以对同一输出目标更换格式，不改输出逻辑。

### 3.3 把“格式规则”拆成多个 `FormatItem`

新增 `%x` 格式时，只需新增一个 `FormatItem` 子类并在 `init()` 注册映射。  
不需要改已有 `MessageFormatItem` 等逻辑，符合开闭原则。

### 3.4 面向抽象编程

`Logger` 只依赖 `LogAppender` 抽象；`Formatter` 只依赖 `FormatItem` 抽象。  
调用方面对接口，减少实现细节传播。

---

## 4. 为什么大量使用多态

日志系统天然有两类“变化点”：

- 输出目标会变（console/file/socket/remote）
- 格式字段会变（`%m/%p/%d/...`，未来还会新增）

多态的价值是把变化封装成独立类，通过统一接口调用：

- 不需要在一个大 `switch` 里堆所有分支
- 新功能通过“新增类 + 注册”完成
- 老代码稳定，回归风险更低

这也是日志库可扩展性的核心。

---

## 5. 一条日志从产生到落盘的完整链路

1. 业务产生 `LogEvent`
2. 调用 `Logger::log(level, event)`
3. `Logger` 做级别过滤
4. 遍历 `m_appenders`，调用每个 `appender->log(...)`
5. `Appender` 内部调用 `m_formatter->format(...)`
6. `Formatter` 遍历 `m_items`，逐项写入 `stringstream`
7. 得到字符串后由 `Appender` 输出到目标（终端/文件）

这条链路体现了“调度（Logger）—格式（Formatter）—输出（Appender）”三层分离。

---

## 6. 日志库的核心到底是什么

如果只保留一个“最核心抽象”，就是：

- **把日志事件流，经由可组合策略（格式 + 输出），稳定地写出去。**

其中最关键的工程能力是：

- 低耦合扩展（新格式、新目标）
- 运行时稳定（不影响业务主流程）
- 并发正确性（多线程不乱序、不崩溃）

---

## 7. 多线程必须考虑的问题（非常重要）

你当前代码还没有加锁，这在真实项目里会有并发风险。

### 7.1 共享容器并发访问

- `Logger::m_appenders` 在 `add/del/log` 并发时可能迭代器失效、崩溃
- 方案：读写锁（读多写少）或互斥锁保护

### 7.2 文件输出并发写入

- 多线程同时写 `std::ofstream` 可能交叉写、日志行打碎
- 方案：`FileLogAppender` 内部加锁；或单独异步落盘线程

### 7.3 标准输出并发交错

- 多线程 `std::cout` 可能半行交叉
- 方案：`StdioLogAppender` 加锁，保证“一次日志一把锁”

### 7.4 格式器与配置热更新

- 运行中替换 `m_formatter` 时，若无同步可能野指针/竞态
- 方案：原子替换 `shared_ptr` 或在锁内读写

### 7.5 性能与阻塞

- 同步 I/O 会拖慢业务线程（尤其慢磁盘或网络输出）
- 常见方案：异步日志（无锁队列/有界队列 + 后台刷盘线程）

### 7.6 可靠性边界

- 日志系统内部异常不能反噬业务
- 方案：Appender 内部捕获异常、降级输出、失败计数告警

---

## 8. 你这个实现的下一步建议（按优先级）

1. 先修正接口一致性（`Logger::log` 与 `Appender::log` 参数类型一致）
2. 补全 `LogFormatter::init()` 的 token 到 `FormatItem` 的映射
3. 给 `Logger` 和各 `Appender` 加基础互斥锁
4. 增加最小单元测试：`%m/%p/%d{}` 与多线程并发写
5. 再考虑异步日志化（性能优化阶段）

---

## 9. 一个最小心智模型（记住这个就够了）

- `LogEvent`：我记录了什么
- `Logger`：该不该记，发给谁
- `Formatter`：要怎么排版
- `Appender`：最终写到哪里

四者各司其职，就是“低耦合 + 可扩展”的来源。
