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
#include <QListWidget>
#include <QListWidgetItem>
#include <QSlider>
#include <QStyle>
#include <QFileIconProvider>
#include <QTimer>
#include <iostream>

const QString SOCKET_NAME = "kde_sashimi_cpp_single_instance";

class AutoFitLabel : public QLabel {
public:
    AutoFitLabel(const QPixmap &pixmap, QWidget *parent = nullptr) : QLabel(parent), m_pixmap(pixmap) {
        setAlignment(Qt::AlignCenter);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        setMinimumSize(50, 50);
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

        // [แก้ไขที่นี่] ใช้ QTimer หน่วงเวลา 0 วินาที เพื่อให้รอ app.exec() เริ่มงานก่อน
        // ป้องกันแอปแครชจากการพยายามดึงไฟล์ไอคอนตอนโปรแกรมเพิ่งเริ่ม
        QTimer::singleShot(0, this, [this, filePath]() {
            loadFile(filePath, false);
        });
    }

    void updateFolderFileList() {
        QFileInfo current(m_filePath);
        QDir parentDir = current.isDir() ? current.dir() : current.dir();
        m_folderFiles = parentDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
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
        if (QFileInfo::exists(m_filePath)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath));
        }
    }

    void clearLayout() {
        if (m_player) {
            m_player->stop();
            delete m_player;
            m_player = nullptr;
        }

        if (m_officeProcess) {
            m_officeProcess->kill();
            m_officeProcess->deleteLater();
            m_officeProcess = nullptr;
        }
        
        if (m_currentPdfDoc) {
            delete m_currentPdfDoc;
            m_currentPdfDoc = nullptr;
        }
        m_pdfScrollLayout = nullptr;
        m_pdfLoadingLabel = nullptr;

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
        
        QString cleanPath = filePath;
        if (cleanPath.startsWith("file://")) {
            cleanPath = QUrl(cleanPath).toLocalFile();
        }
        
        m_filePath = QFileInfo(cleanPath).absoluteFilePath();
        updateFolderFileList();

        QFileInfo fi(m_filePath);
        setWindowTitle(QString("kde-sashimi: %1").arg(fi.fileName()));
        
        int totalItems = m_folderFiles.size();
        int currentIndex = (m_currentFileIndex >= 0) ? m_currentFileIndex + 1 : 0;
        m_fileInfoLabel->setText(QString("%1 (%2/%3)").arg(fi.fileName()).arg(currentIndex).arg(totalItems));

        if (syncToDolphin) {
            QDBusInterface fm("org.freedesktop.FileManager1", "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1", QDBusConnection::sessionBus());
            if (fm.isValid()) {
                QStringList uris;
                uris << QUrl::fromLocalFile(m_filePath).toString();
                fm.call("ShowItems", uris, QString(""));
            }
        }

        if (!fi.exists()) {
            QLabel *label = new QLabel(QString("File or Directory not found:\n%1").arg(m_filePath), this);
            label->setAlignment(Qt::AlignCenter);
            m_layout->addWidget(label);
            show();
            raise();
            activateWindow();
            return;
        }

        if (fi.isDir()) {
            previewFolder();
            show();
            raise();
            activateWindow();
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
    QWidget* createMediaControls(QMediaPlayer *player, QAudioOutput *audioOutput) {
        QWidget *controlWidget = new QWidget(this);
        QHBoxLayout *controlLayout = new QHBoxLayout(controlWidget);
        controlLayout->setContentsMargins(0, 5, 0, 0);

        QPushButton *btnPlayPause = new QPushButton(style()->standardIcon(QStyle::SP_MediaPause), "", controlWidget);
        QPushButton *btnStop = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop), "", controlWidget);
        QSlider *seekSlider = new QSlider(Qt::Horizontal, controlWidget);
        QLabel *lblTime = new QLabel("00:00 / 00:00", controlWidget);
        QSlider *volumeSlider = new QSlider(Qt::Horizontal, controlWidget);
        
        volumeSlider->setRange(0, 100);
        volumeSlider->setValue(qRound(audioOutput->volume() * 100));
        volumeSlider->setMaximumWidth(100);

        controlLayout->addWidget(btnPlayPause);
        controlLayout->addWidget(btnStop);
        controlLayout->addWidget(seekSlider);
        controlLayout->addWidget(lblTime);
        controlLayout->addWidget(new QLabel("🔊", controlWidget));
        controlLayout->addWidget(volumeSlider);

        auto formatTime = [](qint64 ms) -> QString {
            qint64 s = ms / 1000;
            qint64 m = s / 60;
            s = s % 60;
            return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        };

        connect(btnPlayPause, &QPushButton::clicked, [player, btnPlayPause, this]() {
            if (player->playbackState() == QMediaPlayer::PlayingState) {
                player->pause();
                btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            } else {
                player->play();
                btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            }
        });

        connect(btnStop, &QPushButton::clicked, [player, btnPlayPause, this]() {
            player->stop();
            btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        });

        connect(player, &QMediaPlayer::positionChanged, [seekSlider, lblTime, formatTime, player](qint64 position) {
            if (!seekSlider->isSliderDown()) {
                seekSlider->setValue(position);
            }
            lblTime->setText(QString("%1 / %2").arg(formatTime(position)).arg(formatTime(player->duration())));
        });

        connect(player, &QMediaPlayer::durationChanged, [seekSlider, lblTime, formatTime](qint64 duration) {
            seekSlider->setRange(0, duration);
            lblTime->setText(QString("00:00 / %1").arg(formatTime(duration)));
        });

        connect(seekSlider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);

        connect(volumeSlider, &QSlider::valueChanged, [audioOutput](int value) {
            audioOutput->setVolume(value / 100.0);
        });

        return controlWidget;
    }

    void previewFolder() {
        QDir dir(m_filePath);
        QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

        qint64 totalSize = 0;
        int fileCount = 0;
        int dirCount = 0;

        QListWidget *listWidget = new QListWidget(this);
        listWidget->setViewMode(QListWidget::IconMode);
        listWidget->setIconSize(QSize(64, 64));
        listWidget->setResizeMode(QListWidget::Adjust);
        listWidget->setMovement(QListView::Static);
        listWidget->setSpacing(12);
        listWidget->setWordWrap(true);
        listWidget->setStyleSheet(
            "QListWidget { font-size: 12px; padding: 10px; background-color: transparent; border: none; }"
            "QListWidget::item { padding: 8px; border-radius: 6px; }"
            "QListWidget::item:hover { background-color: rgba(128, 128, 128, 0.2); }"
        );

        QFileIconProvider iconProvider;

        for (const QFileInfo &entry : entries) {
            QString sizeStr;
            if (entry.isDir()) {
                dirCount++;
            } else {
                fileCount++;
                totalSize += entry.size();
                double kb = entry.size() / 1024.0;
                if (kb < 1024) {
                    sizeStr = QString::number(kb, 'f', 1) + " KB";
                } else {
                    sizeStr = QString::number(kb / 1024.0, 'f', 1) + " MB";
                }
            }

            QIcon icon = iconProvider.icon(entry);
            QString itemText = entry.fileName();
            if (!entry.isDir()) {
                itemText += "\n(" + sizeStr + ")";
            }

            QListWidgetItem *item = new QListWidgetItem(icon, itemText, listWidget);
            item->setData(Qt::UserRole, entry.absoluteFilePath());
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
            item->setToolTip(entry.fileName() + (entry.isDir() ? "" : " - " + sizeStr));
        }

        connect(listWidget, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
            QString selectedPath = item->data(Qt::UserRole).toString();
            if (!selectedPath.isEmpty()) {
                loadFile(selectedPath, true);
            }
        });

        QLabel *statsLabel = new QLabel(QString("📁 <b>%1</b><br/>📊 %2 Folders, %3 Files (Total Size: %4 MB)")
                                           .arg(dir.dirName().isEmpty() ? m_filePath : dir.dirName())
                                           .arg(dirCount)
                                           .arg(fileCount)
                                           .arg(QString::number(totalSize / (1024.0 * 1024.0), 'f', 2)), this);
        statsLabel->setStyleSheet("font-size: 14px; padding: 12px; color: #eee; background-color: rgba(0, 0, 0, 0.3); border-radius: 6px; margin-bottom: 5px;");

        m_layout->addWidget(statsLabel);
        m_layout->addWidget(listWidget);
    }

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

        QLabel *loadingLabel = new QLabel(QString("⏳ กำลังเตรียมการพรีวิวไฟล์ %1...\n(กระบวนการนี้ใช้เวลาสักครู่เฉพาะการเปิดครั้งแรก)").arg(fileInfo.fileName()), this);
        loadingLabel->setAlignment(Qt::AlignCenter);
        loadingLabel->setStyleSheet("font-size: 16px; color: #999; font-weight: bold;");
        m_layout->addWidget(loadingLabel);

        QString profileDir = cacheDir + "/lo_profile_" + hashKey;
        QDir().mkpath(profileDir);

        m_officeProcess = new QProcess(this);
        QStringList args;
        args << QString("-env:UserInstallation=file://%1").arg(profileDir)
             << "--headless"
             << "--convert-to" << "pdf"
             << m_filePath
             << "--outdir" << cacheDir;

        QString baseName = fileInfo.completeBaseName();
        QString originalFilePath = m_filePath;

        connect(m_officeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, originalFilePath, cacheDir, baseName, cachedPdfPath, profileDir]() {
            if (m_filePath != originalFilePath) return;

            QString generatedPdf = cacheDir + "/" + baseName + ".pdf";
            clearLayout(); 

            if (QFile::exists(generatedPdf)) {
                if (generatedPdf != cachedPdfPath) {
                    QFile::rename(generatedPdf, cachedPdfPath);
                }
                QDir(profileDir).removeRecursively();
                previewPdf(cachedPdfPath);
            } else {
                QLabel *errorLabel = new QLabel("❌ ไม่สามารถแปลงไฟล์ Office ได้\nโปรดตรวจสอบว่าติดตั้ง LibreOffice แล้วหรือไม่", this);
                errorLabel->setAlignment(Qt::AlignCenter);
                errorLabel->setStyleSheet("font-size: 14px; color: #ff6666;");
                m_layout->addWidget(errorLabel);
            }

            if (m_officeProcess) {
                m_officeProcess->deleteLater();
                m_officeProcess = nullptr;
            }
        });

        m_officeProcess->start("libreoffice", args);
    }

    void previewPdf(const QString &pdfPath) {
        m_currentPdfDoc = new QPdfDocument(this);
        if (m_currentPdfDoc->load(pdfPath) == QPdfDocument::Error::None && m_currentPdfDoc->pageCount() > 0) {
            QScrollArea *scrollArea = new QScrollArea(this);
            scrollArea->setWidgetResizable(true);

            QWidget *container = new QWidget();
            m_pdfScrollLayout = new QVBoxLayout(container);
            m_pdfScrollLayout->setAlignment(Qt::AlignHCenter);
            m_pdfScrollLayout->setSpacing(10);

            container->setLayout(m_pdfScrollLayout);
            scrollArea->setWidget(container);
            m_layout->addWidget(scrollArea);

            m_pdfTotalPages = m_currentPdfDoc->pageCount();
            m_pdfCurrentPage = 0;

            m_pdfLoadingLabel = new QLabel(QString("⏳ กำลังเรนเดอร์หน้า (0/%1)...").arg(m_pdfTotalPages));
            m_pdfLoadingLabel->setAlignment(Qt::AlignCenter);
            m_pdfLoadingLabel->setStyleSheet("font-size: 14px; color: #aaa; font-weight: bold; padding: 15px;");
            m_pdfScrollLayout->addWidget(m_pdfLoadingLabel);

            QString currentPath = m_filePath;
            QTimer::singleShot(5, this, [this, currentPath]() {
                renderNextPdfPage(currentPath);
            });
        } else {
            QLabel *errLabel = new QLabel("❌ เกิดข้อผิดพลาดในการโหลด PDF", this);
            errLabel->setAlignment(Qt::AlignCenter);
            errLabel->setStyleSheet("font-size: 14px; color: #ff6666;");
            m_layout->addWidget(errLabel);
        }
    }

    void renderNextPdfPage(const QString &expectedPath) {
        if (expectedPath != m_filePath || !m_currentPdfDoc || !m_pdfScrollLayout) return;

        if (m_pdfCurrentPage < m_pdfTotalPages) {
            QSize pageSize = m_currentPdfDoc->pagePointSize(m_pdfCurrentPage).toSize() * 1.5;
            QImage img = m_currentPdfDoc->render(m_pdfCurrentPage, pageSize);
            
            QLabel *pageLabel = new QLabel();
            pageLabel->setPixmap(QPixmap::fromImage(img));
            pageLabel->setStyleSheet("border: 1px solid #444; background-color: white;");
            
            if (m_pdfLoadingLabel) {
                int index = m_pdfScrollLayout->indexOf(m_pdfLoadingLabel);
                m_pdfScrollLayout->insertWidget(index, pageLabel);
                
                m_pdfCurrentPage++;
                m_pdfLoadingLabel->setText(QString("⏳ กำลังเรนเดอร์หน้า %1 จาก %2...").arg(m_pdfCurrentPage).arg(m_pdfTotalPages));
            }

            QTimer::singleShot(5, this, [this, expectedPath]() { 
                renderNextPdfPage(expectedPath); 
            });
        } else {
            if (m_pdfLoadingLabel) {
                m_pdfLoadingLabel->deleteLater();
                m_pdfLoadingLabel = nullptr;
            }
        }
    }

    void previewVideo() {
        m_player = new QMediaPlayer(this);
        QAudioOutput *audioOutput = new QAudioOutput(this);
        audioOutput->setVolume(0.5);
        m_player->setAudioOutput(audioOutput);

        QVideoWidget *videoWidget = new QVideoWidget(this);
        videoWidget->setAspectRatioMode(Qt::KeepAspectRatio); 
        
        m_layout->addWidget(videoWidget, 1);
        m_layout->addWidget(createMediaControls(m_player, audioOutput), 0);

        m_player->setVideoOutput(videoWidget);
        m_player->setSource(QUrl::fromLocalFile(m_filePath));
        m_player->play();
    }

    void previewAudio() {
        QLabel *title = new QLabel(QString("🎵 Playing: <b>%1</b>").arg(QFileInfo(m_filePath).fileName()), this);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 16px; margin: 20px;");
        m_layout->addWidget(title, 1);

        m_player = new QMediaPlayer(this);
        QAudioOutput *audioOutput = new QAudioOutput(this);
        audioOutput->setVolume(0.5);
        m_player->setAudioOutput(audioOutput);

        m_layout->addWidget(createMediaControls(m_player, audioOutput), 0);

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

    QWidget *m_contentWidget = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QLabel *m_fileInfoLabel = nullptr;
    QPushButton *m_btnPrev = nullptr;
    QPushButton *m_btnNext = nullptr;
    QPushButton *m_btnOpen = nullptr;

    QString m_filePath;
    QStringList m_folderFiles;
    int m_currentFileIndex = -1;
    
    QMediaPlayer *m_player = nullptr;
    QProcess *m_officeProcess = nullptr; 

    QPdfDocument *m_currentPdfDoc = nullptr;
    QVBoxLayout *m_pdfScrollLayout = nullptr;
    QLabel *m_pdfLoadingLabel = nullptr;
    int m_pdfCurrentPage = 0;
    int m_pdfTotalPages = 0;
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: kde-sashimi <file_path_or_dir>" << std::endl;
        return 1;
    }

    QApplication app(argc, argv);
    QString targetFile = argv[1];
    if (targetFile.startsWith("file://")) {
        targetFile = QUrl(targetFile).toLocalFile();
    }
    targetFile = QFileInfo(targetFile).absoluteFilePath();

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
                if (newFile.startsWith("file://")) {
                    newFile = QUrl(newFile).toLocalFile();
                }
                if (!newFile.isEmpty() && QFileInfo::exists(newFile)) {
                    preview.loadFile(newFile, false);
                }
                clientSocket->disconnectFromServer();
            });
        }
    });

    return app.exec();
}

#include "main.moc"
