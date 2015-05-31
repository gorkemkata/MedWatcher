# MedWatcher
Real Time Motion Processing

Real time image processing with Opencv. 

Program watches a person who is walking, and if s/he falls, the program detects it and sends an e-mail
to user defined e-mail address. This can be used to watch elderly people who live alone.


To build project:
You need to link opencv libraries and openssl libraries with your compilers linker.

I used the below project to send e-mails within C++
http://www.codeproject.com/Articles/98355/SMTP-Client-with-SSL-TLS

Here is my prezi presentation about the project: ( in turkish )
https://prezi.com/tiotsrlk7jrg/medwatcher/

Here is the executable of the project:
https://drive.google.com/file/d/0By8TkYdgMAApcTdWanNXS2NFZkU/view
  (It sends e-mails to the specified address from medwatcherproject@gmail.com)
