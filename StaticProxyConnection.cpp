#include "StaticProxyConnection.h"
#include <QHostAddress>
#include <QtNetwork/QSslCipher>

StaticProxyConnection::StaticProxyConnection(QObject *parent): QObject(parent)
{
    mSocket = new QSslSocket(this);
    connect(mSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    connect(mSocket, SIGNAL(encrypted()), this, SLOT(socketEncrypted()));
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(mSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(readReady()));
    connect(parent, SIGNAL(stop()), this, SLOT(stop()));

    // Certificte pinning. This tells Qt that it should trust the specified certificate
    QList<QSslCertificate> pseudoTrustedCA = QSslCertificate::fromPath(":/certs/SP-server-certificate.pem");
    if (pseudoTrustedCA.empty()) {
        qFatal("Error: No Trusted Certificate Authorities");
    }
    mSocket->setCaCertificates(pseudoTrustedCA);

    // Specify that we are only using the TLSv1 protocol
    mSocket->setProtocol(QSsl::TlsV1_0);
}

/**
 * Makes a connection to the static proxy.
 * @brief StaticProxyConnection::connectTo
 * @param host
 * @param port
 */
void StaticProxyConnection::connectTo(const QString &host, quint16 port){
    qDebug() << "Connecting...";

    // This call is non-blocking, therefore we will need to wait for the connection
    mSocket->connectToHostEncrypted(host, port);
    if(!mSocket->waitForConnected(100)){
        qDebug() << "Error: " << mSocket->errorString();
    }
}

/**
 * Read from the encrypted socket.
 * @brief StaticProxyConnection::read
 * @return byte array containg the received data
 */
QByteArray StaticProxyConnection::read(){
    QByteArray incomingData;
    if(mSocketState == QAbstractSocket::ConnectedState && mSocket->isEncrypted()){
        incomingData = mSocket->read(mSocket->encryptedBytesAvailable());
    }
    // TODO: This may return null but okay for now.
    return incomingData;
}

/**
 * Write to the encrypted socket.
 * @brief StaticProxyConnection::write
 * @param dataToWrite
 */
void StaticProxyConnection::write(const QByteArray &dataToWrite){
    if(mSocketState == QAbstractSocket::ConnectedState && mSocket->isEncrypted()){
        qDebug() << "Writting " << dataToWrite;
        mSocket->write(dataToWrite);
    }
}

/**
 * Gets the number of bytes available in the socket buffer to be read.
 * @brief StaticProxyConnection::bytesAvailable
 * @return number of bytes currently in the socket buffer
 */
qint64 StaticProxyConnection::bytesAvailable(){
    return mSocket->encryptedBytesAvailable();
}

/**
 * Check if the socket is currently open.
 * @brief StaticProxyConnection::isOpen
 * @return true if the socket is open, false otherwise.
 */
bool StaticProxyConnection::isOpen(){
    return mSocket->isOpen();
}

/**
 * Disconnect from the static proxy
 * @brief StaticProxyConnection::disconnect
 */
void StaticProxyConnection::disconnect(){
    qDebug() << "Disconnected.";
    if(mSocket != NULL){
        delete mSocket;
        mSocket = NULL;
    }
}

/** --- SIGNALS --- **/

void StaticProxyConnection::readReady(){
    emit readyRead();
}


/** --- SLOTS --- **/
void StaticProxyConnection::socketStateChanged(QAbstractSocket::SocketState state){
    mSocketState = state;
}

/**
 * [Slot] Called when the socket makes a connection, however the socket
 * is not yet encrypted.
 * @brief StaticProxyConnection::connected
 */
void StaticProxyConnection::connected(){
    qDebug() << "Connected. Waiting for an encrypted connection.";

}

/**
 * [Slot] Called when the socket connection has been upgraded to an
 * encrypted connection.
 * @brief StaticProxyConnection::socketEncrypted
 */
void StaticProxyConnection::socketEncrypted(){
    qDebug() << "Connection encrypted!";

    // Code below is only used for debugging purposes
    QSslCipher sessionCipher = mSocket->sessionCipher();
    QString availableCipher = QString("Available Ciphers > Auth:%1, Cipher:%2 ( Used:%3 / Supported:%4 )")
                        .arg(sessionCipher.authenticationMethod())
                        .arg(sessionCipher.name()).arg(sessionCipher.usedBits()).arg(sessionCipher.supportedBits());;
    qDebug() << availableCipher;
}

void StaticProxyConnection::bytesWritten(qint64 bytes){
    qDebug() << bytes << " bytes written...";
}

void StaticProxyConnection::socketError(QAbstractSocket::SocketError){
    qDebug() << "Socket Error: " << mSocket->errorString();

}

void StaticProxyConnection::sslErrors(const QList<QSslError> &errors){
    qDebug() << "SSL Error on: ";
    foreach (const QSslError &error, errors)
        qDebug() << error.errorString();

}

void StaticProxyConnection::stop(){
    // needs implementation
}
