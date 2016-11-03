RcptProxy - SMTP protocol sink for SMTP service in IIS 

version 1.2.0.154, 24 April 2005


Copyright (C) Bronislaw (Bronek) Kozicki 2004-2016. Use, modification and
distribution are subject to the Common Public License Version 1.0
See license.txt or http://www.opensource.org/licenses/cpl.php


This is SMTP protocol sink implementing ISmtpInCommandSink interface,
designed to be used with SMTP service built into IIS (Internet
Information Services) versions 5 (Windows 2000), 5.1 (Windows XP
Professional) and 6.0 (Windows Server 2003). This sink could be
possibly used also with Exchange Server 2000 and Exchange Server 2003,
but has not been tested in this environment. You can learn more about SMTP
event sinks from http://support.microsoft.com/?kbid=324021 or
http://www.microsoft.com/technet/prodtechnol/exchange/2000/maintain/smtpserv.mspx or
http://msdn.microsoft.com/library/en-us/dnsmtps/html/transportevents.asp or
http://msdn.microsoft.com/library/en-us/smtpevt/html/5c031f56-29bf-4fcb-abf6-3eab6789a5bf.asp


Installation and uninstallation:

Installation of RcptProxy consists of 5 steps:
1. create destination directory for application files; suggested one is
  C:\WINDOWS\system32\inetsrv\rcptproxy. Make sure only administrators may
  write in this directory; SYSTEM account (i.e. LocalSystem) requires read
  access.
2. copy *.vbs and *.dll files (register.vbs, unregister.vbs,
  rcptproxy.dll, msvcr71.dll, msvcp71.dll) to the directory created in 1.
  It is recommended that you also copy *.txt files, as documentation aid.
  Visual C++ runtime files (msvcr71.dll and msvcp71.dll) may be also required.
3. run regsvr32 rcptproxy.dll. This will register COM class
  "RcptProxy.Sink.1" in registry.
4. edit register.vbs; as minimum you need to enter IP address of internal
  SMTP server. For details see "Configuration" below
5. run register.vbs

There's no need to restart IIS when installing RcptProxy.

Unistallation is also simple:
1. make sure that unregister.vbs contains identical SMTP instance and
  binding GUID as in register.vbs (see "Configuration" below)
2. run unregister.vbs
3. run regsvr32 /u rcptproxy.dll
4. restart IIS (this will release rcptproxy.dll file blocked by IIS)
5. remove application files
6. remove application directory

You may make several copies of register.vbs and unregister.vbs in order
to use RcptProxy with many instances of SMTP service. Of course to
fully uninstall RcptProxy you will need to execute steps 1. and 2.
for each instance. It is recommended that each instance should employ
unique binding GUID.


Configuration:

RcptProxy is a so called "SMTP event sink", i.e. "plugin" into IIS SMTP
service. In order for IIS to run it, it has to be configured (i.e.
registered) in the IIS metabase. You can perform this registration using
register.vbs script. Before you run it, please verify parameters passed
to Register function. These parameters are (in order):
[N]  instance of SMTP service where sink should be installed;
[M]  binding GUID (just some really unique ID);
[E]  sink enabled;
[P]  sink priority;
[S]  IP address of SMTP service performing verification
Make sure that two first parameters ([N] and [M]) are set to identical
values in unregister.vbs script. You will need it to uninstall RcptProxy,
should you want. After uninstallation the component will be held in IIS
memory until you restart IIS service (IISAdmin).

After sink has been registered, it will be run by IIS SMTP service
to verify each incoming "RCPT TO" command. This verification is
performed by some other SMTP service (e.g. some other SMTP server running
inside your network). The main task of RcptProxy is to pass each RCPT TO
command received by server where it's installed to the other server, read
its reply and then act accordingly - allow RCPT TO or reject it,
optionally also disconnecting the client that issued it. If verification
cannot be completed (e.g. internal SMTP server performing the verification
is unavailable, very busy, response is slow etc.) RCPT TO will be
accepted. You might need to configure it in order to receive some
meaningful result of this verification - this configuration is in the
metabase. You can easily edit the metabase using graphical tools
provided free by Microsoft ("Metabase Explorer" or "MetaEdit").
Be aware that carelessly edited metabase may disable your IIS, or even
worse, may expose your computer to attacks from the Internet.

In order to edit configuration, first you need to find the metabase
path where component "RcptProxy.Sink" has been registered (it's inside
metabase key named "SinkClass"). This path should be similar to given below:
/LM/SmtpSvc/[N]/EventManager/EventTypes
  /{F6628C8D-0D5E-11d2-AA68-00C04FA35B82}/Bindings/[M]/
where:
[N] is instance of SMTP service set in register.vbs script
[M] is binding GUID (just some unique ID) set in register.vbs script

If you can find it, that means that sink has been registered. From there
you can enable or disable it (key "Enabled" nearby) and set its priority
(key "SourceProperties/Priority"). Priority defines the order in which
sinks are called by the SMTP service and is important if you have other
event sinks (e.g. antivirus or antispam), or if RcptProxy is running on
Exchange Server; sink with the lowest number has the highest priority and
will be run first. You may find both values "Enabled" and "Priority" in
register.vbs ([E] and [P]). Finally you will find "SinkProperties/
Configuration".  This is where actual configuration of RcptProxy sink is
stored. Here is complete list of configuration values you can set there:
0 (String) - IP of SMTP server to verify all incoming RCPT TO
  commands (internal SMTP server). This is the last value you can set in
  register.vbs script [S] and is required for the sink to run. If it is
  not set or invalid, RcptProxy will accept all incoming RCPT TO commands.
  This is not a host name - you must use actual IP address;
1 (DWORD) - set any value other than 0 to force configuration refresh. It
  will be reset to 0 by RcptProxy while full configuration is being read.
  RcptProxy does not read complete configuration from the metabase for
  each incoming RCPT command; instead it reads only few values (0, 1 and
  65553). Complete configuration is read under following circumstances: IP
  address or port of internal SMTP server has changed; new connection to
  internal SMTP server is being established or configuration value 1 has
  been set. RcptProxy will reset this value when reading complete
  configuration from the metabase;
65553 (DWORD) - port of internal SMTP server, will default to 25 if not
  set;
65554 (String) - "HELO" command sent from proxy when establishing
  connection to internal SMTP server. It will help you to identify proxied
  requests in the log of the other server. It will default to "world" if not
  set;
65555 (DWORD) - idle timeout in seconds, will default to 300. If
  connection to the other SMTP server remains idle for this time, it will
  be disconnected. Following RCPT verification request will establish
  new connection;
65556 (DWORD) - maximum connection time in seconds, will default to 1800
  if not set. Connection to the other SMTP server will be closed after
  this time. New verification request will establish new connections. This
  setting is useful when you want to make sure that busy RcptProxy won't
  keep single connection to internal SMTP server for very long period of
  time. Both settings (this and 65555) dictate connection caching policy.
  Connection will be also reset when RcptProxy is forced to refresh its
  configuration (configuration value 1). Single instance of RcptProxy will
  create one connection to internal SMTP server in order to perform RCPT
  verification, but IIS may create multiple instances of event sink;
65557 (DWORD) - force disconnection of the client being verified if RCPT
  is rejected by internal SMTP server (0 = false, -1 = true). It will
  default to true if not set. Do not use other values that 0 or -1, as
  these might have special meaning in future versions of RcptProxy;
65558 (String) - "MAIL FROM" command that should be send from proxy to
  internal SMTP server just before each verification of RCPT. If not set
  will default to "rcptproxy@localhost";
65559 (DWORD) - maximum time in milliseconds allowed for the verification
  If verification cannot be completed within this time (e.g. internal SMTP
  server is too busy), sink will accept incoming RCPT command without
  completing verification. If not set will default to 3000 (that is 3
  seconds);
65560 (MultiString) - list of IP addresses which should bypass RcptProxy.
  SMTP communication comming from these IPs will be excluded from RCPT 
  verification. Put each IP in separate line; network masks are not 
  supported (yet). Sink registration will set this list to contain
  127.0.0.1 and IP address of internal SMTP server (configuration value
  0). You may remove it if you want (not recommended) or append additional
  IP addresses. This setting is intended to allow single IIS to act as
  screening SMTP of both incoming and outgoing email - all SMTP
  communication from outside will be verified by RcptProxy against
  internal SMTP server, while communication coming from internal SMTP
  server will not (assuming its IP address is present in this list).
65561 (String) - error message to be sent to client when recipient has
  been determined to be invalid. Place @ character at the end of this
  string if you want recipient address to be appended to the message.
  Do not put SMTP status code here; it will be prefixed automatically
  using number you provide in 65562 (or 550 by default). This string will
  default to "Unable to relay to @" if not set.
65562 (DWORD) - SMTP error status code to be put on start of the error
  message sent to client when recipient has been determined to be invalid.
  Select number in range between 500 and 599, preferably 550 or 554.
  You might also select number in range between 400 and 499, and IIS will
  treat it slightly differently. This code will default to 550 if not set.


Compilation:

When you build the project, please be aware of the following:
1. you need MSVC71 (i.e. Visual C++ version 7.1, compiler version 13.10);
  you should be also able to build it with MSVC8, but this has not been
  tested yet
2. you need ATL, thus free Visual C++ Toolkit will not suffice. Visual
  C++ .NET 2003 Standard or any edition of Visual Studio .NET 2003 should
  be good enough;
3. this project requires three files from Platform SDK to be copied into
  project directory. These files are MailMsg.idl, Seo.idl and
  SmtpEvent.idl. You can find them inside "vc7\PlatformSDK\include"
  subdirectory in your Visual C++ installation.
4. this project does not use .NET framework, however Metabase Explorer
  does, should you want to use it as configuration tool.


Credits:
  Krzysztof Kleszynski for idea, motivation and testing
  Michael Geiger for suggestions and testing


Comments, questions, suggestions: 
  https://github.com/Bronek/rcptproxy/issues
 
