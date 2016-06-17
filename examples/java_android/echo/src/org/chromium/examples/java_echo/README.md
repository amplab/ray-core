Java Echo Example Applications
==============================

**Usage:**

`cd <mojo>/src`

`mojo/devtools/common/mojo_run --android mojo:java_echo_client --logcat-tags=JavaEchoClient`

**To use the Java Echo Server instead of the native Echo Server:**

`mojo/devtools/common/mojo_run --android mojo:java_echo_client --logcat-tags=JavaEchoClient,JavaEchoServer --url-mappings=mojo:echo_server=mojo:java_echo_server`


**To send a message of your choice:**

`mojo/devtools/common/mojo_run --android mojo:java_echo_client --logcat-tags=JavaEchoClient,JavaEchoServer --url-mappings=mojo:echo_server=mojo:java_echo_server --args-for='mojo:java_echo_client Hello Mojo'`

<!---
If you copy the above commands from this source .md file then do not include the backticks: `
-->

