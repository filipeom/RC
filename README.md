# RC Cloud Backup

Simple networking application that allows users to backup the contetns of a 
specified local directory using a cloud service.
## Getting Started

To install the program simply download the source code:
```
git clone https://github.com/filipeom/RC.git
```
To compile the program run:
```
make all
```
### Prerequisites

You will need the gcc compiler:

Arch linux:
```
sudo pacman -S gcc
```

## Running and testing the program

To run the program simply run the user application:
```
./user [-n CSname] [-p CSport]
 ```
CSname: Central server hostname, if this argument is not specified the program 
assumes the central server is being hosted on the same machine as the user 
application is running.

CSport: The port in which the central server operates, as with the CSname argument.
If this argument is not specified the program assumes the central server is being
run on the same machine as the user application, and uses the default port: 58043.

### Testing the system:

To test the system you first need to run the central server application:
```
./CS [-p CSport]
```
CSport: The central server binds and listens on this port for user connections. If 
this argument is not specified, the program assumes the default port of: 58043.

After running the central server you need to register backup servers:
```
./BS [-n CSname] [-p CSport] [-b BSport]
```
CSname: Central Server hostname, if this argument is not specified the program
assumes the central server is being hosted on the same machine as the backup 
server application.

CSport: The port in which the central server operates, as with the CSname argument, if
this argument is not specified the program assumes the central server is being
run on the same machine as the user application, and uses the default port: 58043.

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Filipe Marques** - *Project Development* - [filipeom](https://github.com/filipeom)
* **Jorge Martins** - *Project Development* - [Jorgecmartins](https://github.com/Jorgecmartins)
* **Paulo Dias** - *Project Development* - [PauloACDias](https://github.com/PauloACDias)

