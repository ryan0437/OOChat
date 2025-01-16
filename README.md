开发环境：Qt5.14.2
默认使用本地环回地址和9999、9998端口。
客户端和服务器的配置分别在clientsocket.cpp和tcpserver.cpp内修改
文件传输使用9998号端口，在filetransfer.cpp内修改。 服务器使用MySql，要更改为自己的配置。
服务器MySql配置在dbmanager.cpp内修改。
