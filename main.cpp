#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QKeySequence>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QPixmap>
#include <QImage>
#include <QPdfDocument>
#include <QScrollArea>
#include <QDesktopServices>
#include <QUrl>
#include <QCryptographicHash>
#include <QDBusInterface>
#include <QDBusReply>
#include <iostream>

const QString SOCKET_NAME = "kde_sashimi_cpp_single_instance";

class AutoFitLabel : public QLabel {
public:
    AutoFitLabel(const QPixmap &pixmap, QWidget *parent = nullptr) : QLabel(parent), m_pixmap(pixmap) {
        setAlignment(Qt::AlignCenter);
        setMinimumSize(200, 200);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        if (!m_pixmap.isNull()) {
            setPixmap(m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

private:
    QPixmap m_pixmap;
};

class SashimiPreview : public QWidget {
    Q_OBJECT
public:
    SashimiPreview(const QString &filePath) : QWidget() {
        resize(950, 800);

        QShortcut *shortcutEsc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
        connect(shortcutEsc, &QShortcut::activated, this, &QWidget::close);

        QShortcut *shortcutSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
        connect(shortcutSpace, &QShortcut::activated, this, &QWidget::close);

        QShortcut *shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
        connect(shortcutLeft, &QShortcut::activated, this, &SashimiPreview::prevFile);

        QShortcut *shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
        connect(shortcutRight, &QShortcut::activated, this, &SashimiPreview::nextFile);

        QShortcut *shortcutUp = new QShortcut(QKeySequence(Qt::Key_Up), this);
        connect(shortcutUp, &QShortcut::activated, this, &SashimiPreview::prevFile);

        QShortcut *shortcutDown = new QShortcut(QKeySequence(Qt::Key_Down), this);
        connect(shortcutDown, &QShortcut::activated, this, &SashimiPreview::nextFile);

        QShortcut *shortcutOpen = new QShortcut(QKeySequence(Qt::Key_Return), this);
        connect(shortcutOpen, &QShortcut::activated, this, &SashimiPreview::openWithDefaultApp);

        QVBoxLayout *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(10, 10, 10, 10);
        rootLayout->setSpacing(8);

        QHBoxLayout *topBar = new QHBoxLayout();
        m_btnPrev = new QPushButton("❮  Previous", this);
        m_btnNext = new QPushButton("Next  ❯", this);
        m_btnOpen = new QPushButton("📂 Open with App", this);

        m_btnPrev->setCursor(Qt::PointingHandCursor);
        m_btnNext->setCursor(Qt::PointingHandCursor);
        m_btnOpen->setCursor(Qt::PointingHandCursor);

        m_btnPrev->setStyleSheet("padding: 6px 14px; font-weight: bold;");
        m_btnNext->setStyleSheet("padding: 6px 14px; font-weight: bold;");
        m_btnOpen->setStyleSheet("padding: 6px 14px; background-color: #2b73af; color: white; font-weight: bold; border-radius: 4px;");

        connect(m_btnPrev, &QPushButton::clicked, this, &SashimiPreview::prevFile);
        connect(m_btnNext, &QPushButton::clicked, this, &SashimiPreview::nextFile);
        connect(m_btnOpen, &QPushButton::clicked, this, &SashimiPreview::openWithDefaultApp);

        m_fileInfoLabel = new QLabel(this);
        m_fileInfoLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #bbb;");

        topBar->addWidget(m_btnPrev);
        topBar->addWidget(m_btnNext);
        topBar->addSpacing(15);
        topBar->addWidget(m_fileInfoLabel, 1);
        topBar->addWidget(m_btnOpen);

        rootLayout->addLayout(topBar);

        m_contentWidget = new QWidget(this);
        m_layout = new QVBoxLayout(m_contentWidget);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_contentWidget->setLayout(m_layout);
        rootLayout->addWidget(m_contentWidget, 1);

        loadFile(filePath, false);
    }

    void updateFolderFileList() {
        QFileInfo current(m_filePath);
        QDir dir = current.dir();
        m_folderFiles = dir.entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
        m_currentFileIndex = m_folderFiles.indexOf(current.fileName());
    }

    void prevFile() {
        if (m_folderFiles.isEmpty()) return;
        m_currentFileIndex--;
        if (m_currentFileIndex < 0) {
            m_currentFileIndex = m_folderFiles.size() - 1;
        }
        QFileInfo current(m_filePath);
        QString targetPath = current.dir().filePath(m_folderFiles[m_currentFileIndex]);
        loadFile(targetPath, true);
    }

    void nextFile() {
        if (m_folderFiles.isEmpty()) return;
        m_currentFileIndex++;
        if (m_currentFileIndex >= m_folderFiles.size()) {
            m_currentFileIndex = 0;
        }
        QFileInfo current(m_filePath);
        QString targetPath = current.dir().filePath(m_folderFiles[m_currentFileIndex]);
        loadFile(targetPath, true);
    }

    void openWithDefaultApp() {
        if (QFile::exists(m_filePath)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath));
        }
    }

    void clearLayout() {
        if (m_player) {
            m_player->stop();
            delete m_player;
            m_player = nullptr;
        }

        QLayoutItem *item;
        while ((item = m_layout->takeAt(0)) != nullptr) {
            if (QWidget *widget = item->widget()) {
                widget->deleteLater();
            }
            delete item;
        }
    }

    void loadFile(const QString &filePath, bool syncToDolphin = false) {
        clearLayout();
        m_filePath = QFileInfo(filePath).absoluteFilePath();
        updateFolderFileList();

        QFileInfo fi(m_filePath);
        setWindowTitle(QString("kde-sashimi: %1").arg(fi.fileName()));
        m_fileInfoLabel->setText(QString("%1 (%2/%3)").arg(fi.fileName()).arg(m_currentFileIndex + 1).arg(m_folderFiles.size()));

        if (syncToDolphin) {
            QDBusInterface fm("org.freedesktop.FileManager1", "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1", QDBusConnection::sessionBus());
            if (fm.isValid()) {
                QStringList uris;
                uris << QUrl::fromLocalFile(m_filePath).toString();
                fm.call("ShowItems", uris, QString(""));
            }
        }

        if (!QFile::exists(m_filePath)) {
            QLabel *label = new QLabel(QString("File not found:\n%1").arg(m_filePath), this);
            label->setAlignment(Qt::AlignCenter);
            m_layout->addWidget(label);
            return;
        }

        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(m_filePath);
        QString mimeName = mime.name();
        QString lowerPath = m_filePath.toLower();

        QStringList officeExts = {".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".odt", ".ods", ".odp", ".rtf", ".csv"};
        bool isOffice = false;
        for (const QString &ext : officeExts) {
            if (lowerPath.endsWith(ext)) { isOffice = true; break; }
        }

        if (isOffice) {
            previewOffice();
        } else if (mimeName.startsWith("image/")) {
            QPixmap pix(m_filePath);
            m_layout->addWidget(new AutoFitLabel(pix, this));
        } else if (mimeName == "application/pdf" || lowerPath.endsWith(".pdf")) {
            previewPdf(m_filePath);
        } else if (mimeName.startsWith("video/")) {
            previewVideo();
        } else if (mimeName.startsWith("audio/")) {
            previewAudio();
        } else {
            previewText();
        }

        show();
        raise();
        activateWindow();
    }

private:
    void previewOffice() {
        QString cacheDir = QDir::tempPath() + "/kde_sashimi_cache";
        QDir().mkpath(cacheDir);

        QFileInfo fileInfo(m_filePath);
        QString hashKey = QString(QCryptographicHash::hash((m_filePath + QString::number(fileInfo.lastModified().toMSecsSinceEpoch())).toUtf8(), QCryptographicHash::Md5).toHex());
        QString cachedPdfPath = cacheDir + "/" + hashKey + ".pdf";

        if (QFile::exists(cachedPdfPath)) {
            previewPdf(cachedPdfPath);
            return;
        }

        QProcess libreoffice;
        libreoffice.start("libreoffice", QStringList() << "--headless" << "--convert-to" << "pdf" << m_filePath << "--outdir" << cacheDir);
        if (libreoffice.waitForFinished(10000)) {
            QString baseName = fileInfo.completeBaseName();
            QString generatedPdf = cacheDir + "/" + baseName + ".pdf";
            
            if (QFile::exists(generatedPdf)) {
                if (generatedPdf != cachedPdfPath) {
                    QFile::rename(generatedPdf, cachedPdfPath);
                }
                previewPdf(cachedPdfPath);
                return;
            }
        }

        QLabel *errorLabel = new QLabel("Failed to convert Office document.", this);
        errorLabel->setAlignment(Qt::AlignCenter);
        m_layout->addWidget(errorLabel);
    }

    void previewPdf(const QString &pdfPath) {
        QPdfDocument *pdfDoc = new QPdfDocument(this);
        if (pdfDoc->load(pdfPath) == QPdfDocument::Error::None && pdfDoc->pageCount() > 0) {
            QScrollArea *scrollArea = new QScrollArea(this);
            scrollArea->setWidgetResizable(true);

            QWidget *container = new QWidget();
            QVBoxLayout *scrollLayout = new QVBoxLayout(container);
            scrollLayout->setAlignment(Qt::AlignHCenter);
            scrollLayout->setSpacing(10);

            QSize page0Size = pdfDoc->pagePointSize(0).toSize() * 1.5;
            QLabel *page0Label = new QLabel();
            page0Label->setPixmap(QPixmap::fromImage(pdfDoc->render(0, page0Size)));
            page0Label->setStyleSheet("border: 1px solid #444;");
            scrollLayout->addWidget(page0Label);

            container->setLayout(scrollLayout);
            scrollArea->setWidget(container);
            m_layout->addWidget(scrollArea);

            int pageCount = pdfDoc->pageCount();
            if (pageCount > 1) {
                for (int i = 1; i < pageCount; ++i) {
                    QSize pageSize = pdfDoc->pagePointSize(i).toSize() * 1.5;
                    QLabel *pageLabel = new QLabel();
                    pageLabel->setPixmap(QPixmap::fromImage(pdfDoc->render(i, pageSize)));
                    pageLabel->setStyleSheet("border: 1px solid #444;");
                    scrollLayout->addWidget(pageLabel);
                }
            }
        } else {
            QLabel *errLabel = new QLabel("Error loading PDF.", this);
            errLabel->setAlignment(Qt::AlignCenter);
            m_layout->addWidget(errLabel);
        }
    }

    void previewVideo() {
        m_player = new QMediaPlayer(this);
        QAudioOutput *audioOutput = new QAudioOutput(this);
        m_player->setAudioOutput(audioOutput);

        QVideoWidget *videoWidget = new QVideoWidget(this);
        m_layout->addWidget(videoWidget);

        m_player->setVideoOutput(videoWidget);
        m_player->setSource(QUrl::fromLocalFile(m_filePath));
        m_player->play();
    }

    void previewAudio() {
        QLabel *title = new QLabel(QString("🎵 Playing: <b>%1</b>").arg(QFileInfo(m_filePath).fileName()), this);
        title->setAlignment(Qt::AlignCenter);
        m_layout->addWidget(title);

        m_player = new QMediaPlayer(this);
        QAudioOutput *audioOutput = new QAudioOutput(this);
        m_player->setAudioOutput(audioOutput);

        m_player->setSource(QUrl::fromLocalFile(m_filePath));
        m_player->play();
    }

    void previewText() {
        QTextEdit *edit = new QTextEdit(this);
        edit->setReadOnly(true);

        QFile file(m_filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            edit->setPlainText(in.read(15000));
        } else {
            edit->setPlainText("Could not open text file.");
        }
        m_layout->addWidget(edit);
    }

    Widget *m_contentWidget = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QLabel *m_fileInfoLabel = nullptr;
    QPushButton *m_btnPrev = nullptr;
    QPushButton *m_btnNext = nullptr;
    QPushButton *m_btnOpen = nullptr;

    QString m_filePath;
    QStringList m_folderFiles;
    int m_currentFileIndex = -1;
    QMediaPlayer *m_player = nullptr;
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: kde-sashimi <file_path>" << std::endl;
        return 1;
    }

    QApplication app(argc, argv);
    QString targetFile = QFileInfo(argv[1]).absoluteFilePath();

    QLocalSocket socket;
    socket.connectToServer(SOCKET_NAME);
    if (socket.waitForConnected(300)) {
        socket.write(targetFile.toUtf8());
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return 0;
    }

    QLocalServer::removeServer(SOCKET_NAME);
    QLocalServer server;
    server.listen(SOCKET_NAME);

    SashimiPreview preview(targetFile);

    QObject::connect(&server, &QLocalServer::newConnection, [&server, &preview]() {
        QLocalSocket *clientSocket = server.nextPendingConnection();
        if (clientSocket) {
            QObject::connect(clientSocket, &QLocalSocket::readyRead, [&preview, clientSocket]() {
                QString newFile = QString::fromUtf8(clientSocket->readAll()).trimmed();
                if (!newFile.isEmpty() && QFile::exists(newFile)) {
                    preview.loadFile(newFile, false);
                }
                clientSocket->disconnectFromServer();
            });
        }
    });

    return app.exec();
}

#include "main.moc"
