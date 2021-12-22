# log4hpp
Logging system for C++17 header only (proof of concept)

# Goal

```
 log::debug("Message arg1={}, arg2={}, arg3={}", a, b, c);
 log::warning("Warning message a={1} b={3} c={2}", a, c ,b);
```

## Contexts


```
 auto ctx = log::makeContext("HeavyCalc h={}",h);
 runHeavyCalc(h);
```

Context can define a context of calculation. It works as bracket which is defined as lifetime of the variable "ctx". You can create unlimited nested contexts


```
 void runHeavyCalc(int h) {
   log::progress("Started");
   ///
   log::progress("Stopped");
  }
```

## Backend

```
	//Create logging backend
	//Backend is object which routes messages to an Appender
	//Appender is object constructed by the backend, you specify appended as template argument
	//Constructor parameters are passed through the constructor of the backend
	log4hpp::Backend<log4hpp::UnixFileRotatedAppender> logBackend(
			"{N:8A} {t} {L} {c} {m}{nl}", //format of log message
			      //{N} - id of message
			      //{t} - timestamp
				  //{L} - main level
			      //{c} - context
			      //{m} - log message
			      //{nl} - new line character(s)
			log4hpp::Level::debug,    //maximum level of detail
			"log/logfile",            //target file
			7);                       //days to keep - log is rotated automatically
	logBackend.install();    //install the backend
```

Backend is template class which accepts an **appender**. Appender sends lines to selected target and 
can perform any extra action with logs.

* **StdErrAppender** - sends log to std.error
* **UnixFileAppender** - can send log to a file or to a opened named pipe. If the logging is controled
  using logrotate, the appender can receive signal to close and reopen the file (after rotation). It 
  only doesn't handle the signal itself, but this is easy to do
* **UnixFileRotatedAppender** - can send log to a file, which is automatically rotated on specified time period (default is 1 day). You can specify format of the timestamp in rotated files. You can specify count of days (periods) how long the logs are kept.

## Lookups

* **{}** - inserts argument one-by-one
* **{:fmt}** - specify formatting flags - depends on type of argument
* **{n}** - insert n-th argument (one-based index) n is number
* **{n:fmt}** - insert n-th argument (one-based index) and specify formatting flags

**Variables active to format line**

* **{t}** - Insert timestamp -> string
* **{c}** - Insert contexts -> string
* **{C}** - Insert contexts in reverse order -> string
* **{T}** - Insert thread id -> unsigned int
* **{N}** - Insert message number (numbered starting by 1 at start of application) -> unsigned int
* **{m}** - Insert message -> string
* **{L}** - Insert name of the main level -> string
* **{l}** - Insert name of the sublevel -> string
* **{k}** - Insert level number -> number
* **{nl}** - new line - platform depend
* **{cr}** - carry return
* **{lf}** - line feed

## Formatting 

### Unsigned integer

**:+nnnC**

* **+** - optional puts '+' sign before text
* **-** - optional puts '-' sign before number
* **n** - number - count of left zeroes
* **C** - format
    * **X** - hex output upper-case
    * **x** - hex outpur lower-case
    * **A** - base62
    * **a** - base32
    * **o** - base8
    * **b** - base2

### Integer

**:+nnnC**

* **+** - puts signature even if the number is positive (+ positive, -negative)
* **-** - puts signature even if the number is positive (- positive, +negative)
rest is similar to unsigned integer

### String

**:[0-9]rbqadej<>-**

* **number** - specify reserved space for the string, if the string is smaller, it pads string with space
* **r** - RAW - disable string sanitization - put bytes 1:1
* **b** - Binary - convert bytes to hex numbers
* **q** - Quotation marks - string in quotation marks - quotation marks in the string are escaped
* **a** - Apostrophe - string in single quotation marks - quotation marks in the string are escaped
* **d** - Double - quotation marks are doubled instead escaped or dotted
* **e** - Escape - escape some non-printable charactes \n \r, escape quotation marks
* **j** - JSON - sanitize string to be valid as JSON string (including UTF-8 to Unicode conversion) 
* **<** - align left
* **>** - align right
* **-** - align right

Default behaviour - "dots" - non-printable and collision characters are dotted, chars > 127 are not sanitized

### Boolean

prints **true** or  **false**

### Double/Float

(not finished yet)

:nnnn

number specifies count of decimal places. Without format, numbers are displayed using printf(%g)

### Lambda functions

```
log::debug("Calculated value: {}", [&]{return 42;});
```

Lamba function is executed only when specified log level is enabled

