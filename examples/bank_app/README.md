# Running bank_app

Bank is a sample application that uses the vanadium principal service to
enable applications to obtain a user identity for authentication. The app
currently only works on android and with go build support.

The customer obtains a user identity through the principal service and then
talks to the bank to deposit/withdraw some money. The bank will only accept
transactions from customers that have a user identity. 

# To run customer
$MOJO_DIR/src/mojo/devtools/common/mojo_run mojo:customer --android

