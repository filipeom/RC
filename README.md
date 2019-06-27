# RC Cloud Backup

Simple networking application that allows users to backup the contents of a 
specified local directory using a cloud service.
## Getting Started
Go to each module, in order:

* [Central Server](central_server/) - The registry server 
* [Backup Server](backup_server/) - The file backup server
* [User](user/) - The user client

And compile the source code with:
```
make
```
### Prerequisites

A linux system to run the binaries.

## Running and testing the program

To start the program simply run the user application as:
```
./user [-n CSname] [-p CSport]
 ```
Where:
* **CSname**-*optional*: is the name of the machine where the central server(CS) runs.
If this argument is omitted, the CS should be running on the same machine.

* **CSport**-*optional*: is the well-known port where the CS server accepts user requests,
in TCP. If omitted, it assumes the value is 58043.

Once the user program is running, it waits for the user to indicate the action to take, notably:
```
login user pass //Authenticates user with CS.
deluser         //Deletes the user that has previously logged in successfully.
backup dir      //Uploads files from - dir.
restore dir     //Downloads files from - dir.
dirlist         //Lists all the directories the user has saved in the cloud.
delete dir      //Deletes files from - dir.
logout          //Allows using the appliction with a different username.
exit            //The user application terminates.
```
### Testing the system:

To test the system you first need to run the central server application as:
```
./CS [-p CSport]
```
where:
* **CSport**-*optional*: is the well-known port where the CS server accepts requests, in 
TCP. If omitted, it assumes the value 58043.

After running the central server you need to register backup servers as:
```
./BS [-n CSname] [-p CSport] [-b BSport]
```
where:
* **BSport**-*optional*: is the well-known port where the BS server accepts TCP requests 
from the user application. If omitted, it assumes the value 59043.
* **CSname**-*optional*: is the name of the machine where the central server (CS) runs.
If this argument is omitted, the CS should be running on the same machine.
* **CSport**-*optional*: is the well-known port where the CS server accepts requests. 
If omitted, it assumes the value 58043.

## Built With

* [Vim](https://www.vim.org/) - The editor used by the authors.

## Authors

* **Filipe Marques** - *Developer* - [filipeom](https://github.com/filipeom)
* **Jorge Martins** - *Developer* - [Jorgecmartins](https://github.com/Jorgecmartins)
* **Paulo Dias** - *Developer* - [PauloACDias](https://github.com/PauloACDias)

