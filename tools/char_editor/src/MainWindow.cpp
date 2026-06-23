#include "MainWindow.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStringList>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QInputDialog>
#include <QSplitter>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QSizePolicy>

#include "CharacterSchema.h"
#include "StringListEditor.h"

#include <array>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
#include <sys/stat.h>
#endif

namespace CharEditor
{

namespace
{

constexpr std::array<const char*, 4> kBackpackSectionKeys{
    "accessories", "consumables", "kits & tools", "general"};
constexpr std::array<const char*, 4> kBackpackTabTitles{
    "Accessories", "Consumables", "Kits && Tools", "General"};

QSpinBox* MakeSpinBox(int min, int max)
{
    QSpinBox* s = new QSpinBox();
    s->setRange(min, max);
    return s;
}

QWidget* WrapTabInScrollArea(QWidget* content)
{
    if (!content) { return content; }
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(content);
    return scroll;
}

// Skills stat groups: 3 columns when wide enough, otherwise 2.
class ProficiencySkillsGrid : public QWidget
{
  public:
    explicit ProficiencySkillsGrid(QWidget* parent = nullptr) : QWidget(parent)
    {
        grid_ = new QGridLayout(this);
        grid_->setContentsMargins(0, 0, 0, 0);
        grid_->setHorizontalSpacing(12);
        grid_->setVerticalSpacing(8);
    }

    void setStatGroups(std::vector<QGroupBox*> groups)
    {
        groups_ = std::move(groups);
        applyLayout();
    }

  protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        applyLayout();
    }

    void showEvent(QShowEvent* event) override
    {
        QWidget::showEvent(event);
        applyLayout();
    }

  private:
    void clearGrid()
    {
        while (QLayoutItem* it = grid_->takeAt(0)) { delete it; }
    }

    void applyLayout()
    {
        if (groups_.empty()) { return; }
        int w = width();
        if (w <= 0 && parentWidget()) { w = parentWidget()->width(); }
        if (w <= 0) { w = 640; }
        const int cols = (w >= 520) ? 3 : 2;

        clearGrid();
        for (size_t i = 0; i < groups_.size(); ++i)
        {
            const int r = static_cast<int>(i) / cols;
            const int c = static_cast<int>(i) % cols;
            grid_->addWidget(groups_[i], r, c);
        }
        for (int c = 0; c < cols; ++c) { grid_->setColumnStretch(c, 1); }
    }

    QGridLayout* grid_ = nullptr;
    std::vector<QGroupBox*> groups_;
};

}

MainWindow::MainWindow(CharacterRepository repoParam) : repo(std::move(repoParam))
{
    setWindowTitle("Character JSON Editor");
    setMinimumSize(480, 360);
    resize(900, 560);

    QToolBar* toolbar = addToolBar("Main");
    QAction* actNew = toolbar->addAction("New");
    QAction* actSave = toolbar->addAction("Save");
    QAction* actSaveAs = toolbar->addAction("Save As");
    QAction* actGenerate = toolbar->addAction("Generate");
    QAction* actViewOnWeb = toolbar->addAction("View on web");

    QObject::connect(actNew, &QAction::triggered, this, &MainWindow::OnToolbarNew);
    QObject::connect(actSave, &QAction::triggered, this, &MainWindow::OnToolbarSave);
    QObject::connect(actSaveAs, &QAction::triggered, this, &MainWindow::OnToolbarSaveAs);
    QObject::connect(actGenerate, &QAction::triggered, this, &MainWindow::OnToolbarGenerate);
    QObject::connect(actViewOnWeb, &QAction::triggered, this, &MainWindow::OnToolbarViewOnWeb);

    QWidget* central = new QWidget();
    setCentralWidget(central);

    QHBoxLayout* root = new QHBoxLayout();
    central->setLayout(root);

    QSplitter* splitter = new QSplitter();
    root->addWidget(splitter);

    QWidget* fileListColumn = new QWidget();
    fileListColumn->setMinimumWidth(static_cast<int>(120 * 0.85));
    fileListColumn->setMaximumWidth(static_cast<int>(260 * 0.85));
    QVBoxLayout* fileListColumnLayout = new QVBoxLayout(fileListColumn);
    fileListColumnLayout->setContentsMargins(0, 0, 0, 0);
    fileListColumnLayout->setSpacing(6);

    validationLabel = new QLabel();
    validationLabel->setWordWrap(true);
    fileListColumnLayout->addWidget(validationLabel, 0);

    QFrame* fileListPanel = new QFrame();
    fileListPanel->setFrameShape(QFrame::StyledPanel);
    fileListPanel->setFrameShadow(QFrame::Sunken);
    QVBoxLayout* fileListPanelLayout = new QVBoxLayout(fileListPanel);
    fileListPanelLayout->setContentsMargins(4, 4, 4, 4);
    fileListPanelLayout->setSpacing(6);
    fileListColumnLayout->addWidget(fileListPanel, 1);

    fileList = new QListWidget();
    fileList->setFrameShape(QFrame::NoFrame);
    fileListPanelLayout->addWidget(fileList, 1);
    QPushButton* refreshFileListButton = new QPushButton("Refresh List");
    refreshFileListButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    fileListPanelLayout->addWidget(refreshFileListButton, 0);
    QObject::connect(refreshFileListButton, &QPushButton::clicked, this, &MainWindow::OnRefreshFileListButton);

    splitter->addWidget(fileListColumn);
    QObject::connect(fileList,
                     &QListWidget::currentItemChanged,
                     this,
                     &MainWindow::OnCharacterFileListCurrentItemChanged);
    fileList->installEventFilter(this);

    QWidget* right = new QWidget();
    splitter->addWidget(right);

    QVBoxLayout* rightLayout = new QVBoxLayout();
    right->setLayout(rightLayout);

    editorStack = new QStackedWidget();
    rightLayout->addWidget(editorStack, 1);

    QWidget* placeholderPage = new QWidget();
    QVBoxLayout* phLayout = new QVBoxLayout(placeholderPage);
    QLabel* placeholderHint =
        new QLabel("Select a character file from the list, or choose New to create one.");
    placeholderHint->setWordWrap(true);
    placeholderHint->setAlignment(Qt::AlignCenter);
    phLayout->addStretch(1);
    phLayout->addWidget(placeholderHint);
    phLayout->addStretch(1);
    editorStack->addWidget(placeholderPage);

    tabs = new QTabWidget();
    // Default style pane padding is often ~8–12px; tighten to ~half for more form space.
    tabs->setStyleSheet(QStringLiteral("QTabWidget::pane { margin: 2px; padding: 4px; }"));
    editorStack->addWidget(tabs);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({static_cast<int>(200 * 0.85), 680 + static_cast<int>(200 * 0.15)});

    BuildTabs();
    doc = CharacterDocument();
    RefreshFileList();
    SetEditorWorkspaceOpen(false);

    QApplication::instance()->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    QApplication::instance()->removeEventFilter(this);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // QListView delivers mouse events to viewport(), not the QListWidget itself.
    const bool onFileListSurface =
        (fileList != nullptr && (watched == fileList || watched == fileList->viewport()));
    if (onFileListSurface)
    {
        if (event->type() == QEvent::MouseButtonPress) { fileListLoadsNeedUserGesture_ = false; }
        else if (event->type() == QEvent::KeyPress)
        {
            switch (static_cast<QKeyEvent*>(event)->key())
            {
            case Qt::Key_Down:
            case Qt::Key_Up:
            case Qt::Key_Home:
            case Qt::Key_End:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
            case Qt::Key_Space:
                fileListLoadsNeedUserGesture_ = false;
                break;
            default:
                break;
            }
        }
        else if (event->type() == QEvent::FocusIn)
        {
            switch (static_cast<QFocusEvent*>(event)->reason())
            {
            case Qt::MouseFocusReason:
            case Qt::TabFocusReason:
            case Qt::BacktabFocusReason:
            case Qt::ShortcutFocusReason:
                fileListLoadsNeedUserGesture_ = false;
                break;
            default:
                break;
            }
        }
    }
    if (event->type() == QEvent::Wheel && qobject_cast<QAbstractSpinBox*>(watched)) { return true; }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::OnToolbarNew()
{
    if (!MaybeDiscardUnsavedChanges()) { return; }
    NewCharacter();
}
void MainWindow::OnToolbarSave() { Save(); }
void MainWindow::OnToolbarSaveAs() { SaveAs(); }

void MainWindow::OnToolbarGenerate()
{
    if (!editorWorkspaceOpen_)
    {
        QMessageBox::information(this, "Generate", "Select a character file or choose New first.");
        return;
    }

    const QString root = ProjectRootAbsolute();
    if (root.isEmpty() || !QDir(root).exists())
    {
        QMessageBox::critical(
            this,
            "Generate",
            QString("Could not resolve the PDF project root from the cfg folder:\n%1\n\n"
                    "Expected that path under the current working directory (repo root, or build/ when using make run_editor).")
                .arg(QString::fromStdString(repo.RootDir())));
        return;
    }

    if (doc.IsDirty())
    {
        const int answer =
            QMessageBox::question(this,
                                  "Generate",
                                  "Save changes before generating PDFs?\n\n"
                                  "The generator reads character JSON files from disk.",
                                  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                  QMessageBox::Save);
        if (answer == QMessageBox::Cancel) { return; }
        if (answer == QMessageBox::Save)
        {
            Save();
            if (doc.IsDirty()) { return; }
        }
    }

    RunPdfGeneratorAndShowResult(root);
}

void MainWindow::OnToolbarViewOnWeb()
{
    OpenCharacterOnWeb();
}

QString MainWindow::ResolveWebrenderBaseUrl() const
{
    const QByteArray env = qgetenv("PDF_WEBRENDER_BASE_URL");
    if (!env.isEmpty())
    {
        QString base = QString::fromUtf8(env).trimmed();
        while (base.endsWith('/'))
        {
            base.chop(1);
        }
        return base;
    }
    return QStringLiteral("https://delthazor.github.io/charPdf");
}

QString MainWindow::DeriveWebrenderSlugFromPath(const QString& filePath) const
{
    const QString baseName = QFileInfo(filePath).fileName();
    if (!baseName.startsWith(QStringLiteral("char_"), Qt::CaseInsensitive)
        || !baseName.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
    {
        return {};
    }
    const QString stem = baseName.left(baseName.size() - 5);
    return stem.mid(5).toLower();
}

void MainWindow::OpenCharacterOnWeb()
{
    if (!editorWorkspaceOpen_)
    {
        QMessageBox::information(
            this,
            QStringLiteral("View on web"),
            QStringLiteral("Select a character file or choose New first."));
        return;
    }

    const std::optional<std::string>& pathOpt = doc.FilePath();
    if (!pathOpt.has_value() || pathOpt->empty())
    {
        QMessageBox::information(
            this,
            QStringLiteral("View on web"),
            QStringLiteral("Save the character as char_<name>.json first, then try again."));
        return;
    }

    const QString filePath = QString::fromStdString(*pathOpt);
    const QString slug = DeriveWebrenderSlugFromPath(filePath);
    if (slug.isEmpty())
    {
        QMessageBox::information(
            this,
            QStringLiteral("View on web"),
            QStringLiteral("The character file must be named char_<name>.json (for example char_Celdrick.json)."));
        return;
    }

    const QString baseUrl = ResolveWebrenderBaseUrl();
    const QUrl url(baseUrl + QStringLiteral("/c/") + slug + QStringLiteral(".html"));
    if (!QDesktopServices::openUrl(url))
    {
        QMessageBox::warning(
            this,
            QStringLiteral("View on web"),
            QStringLiteral("Could not open the default browser for:\n%1").arg(url.toString()));
    }
}

QString MainWindow::ProjectRootAbsolute() const
{
    const QString cfgAbs =
        QDir::cleanPath(QDir(QDir::currentPath()).filePath(QString::fromStdString(repo.RootDir())));
    QFileInfo cfgFi(cfgAbs);
    if (!cfgFi.exists() || !cfgFi.isDir()) { return {}; }

    // make run_editor uses cwd = build/ and assets/cfg via build/assets -> ../assets.
    // Without resolving symlinks, two cdUps stop at build/ instead of the repo root.
    QString cfgCanon = cfgFi.canonicalFilePath();
    if (cfgCanon.isEmpty()) { cfgCanon = cfgAbs; }

    QDir d(cfgCanon);
    if (!d.cdUp()) { return {}; }
    if (!d.cdUp()) { return {}; }
    return d.absolutePath();
}

QString MainWindow::ResolvePdfAppPath(const QString& projectRoot) const
{
    auto absoluteExeIfFile = [](const QString& path) -> QString {
        const QFileInfo fi(path);
        if (!fi.isFile()) { return {}; }
        const QString c = fi.canonicalFilePath();
        return c.isEmpty() ? fi.absoluteFilePath() : c;
    };

    QString standard = QDir(projectRoot).filePath(QStringLiteral("build/pdf_app"));
#if defined(Q_OS_WIN)
    standard += QStringLiteral(".exe");
#endif
    if (QString resolved = absoluteExeIfFile(standard); !resolved.isEmpty()) { return resolved; }

    // Same directory as char_editor when using make run_editor (cwd is build/).
    QString sibling = QDir(QDir::currentPath()).filePath(QStringLiteral("pdf_app"));
#if defined(Q_OS_WIN)
    sibling += QStringLiteral(".exe");
#endif
    if (QString resolved = absoluteExeIfFile(sibling); !resolved.isEmpty()) { return resolved; }

    return standard;
}

namespace
{
QStringList ExtractPdfGeneratorErrorLines(const QString& mergedOutput)
{
    QStringList out;
    const QStringList lines = mergedOutput.split(u'\n', Qt::SkipEmptyParts);
    for (QString line : lines)
    {
        line.remove(u'\r');
        const QString t = line.trimmed();
        if (t.isEmpty()) { continue; }
        if (t.contains(QLatin1String("ERROR:")) || t.startsWith(QLatin1String("Error:"), Qt::CaseInsensitive))
        {
            if (!out.contains(t)) { out.append(t); }
        }
    }
    return out;
}
}

void MainWindow::RunPdfGeneratorAndShowResult(const QString& projectRoot)
{
    const QString pdfApp = ResolvePdfAppPath(projectRoot);
    const QFileInfo exe(pdfApp);
    if (!exe.exists() || !exe.isFile())
    {
        QMessageBox::critical(
            this,
            QStringLiteral("Generate"),
            QStringLiteral("The PDF app was not found:\n%1\n\n"
                           "Build it from the repo root (for example: make or make pdf_app).")
                .arg(pdfApp));
        return;
    }

    const QString buildDir = QDir::cleanPath(QDir(projectRoot).filePath(QStringLiteral("build")));
    if (!QDir(buildDir).exists())
    {
        QMessageBox::critical(this,
                              QStringLiteral("Generate"),
                              QStringLiteral("The build directory was not found:\n%1").arg(buildDir));
        return;
    }

    struct WaitCursor
    {
        WaitCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
        ~WaitCursor() { QApplication::restoreOverrideCursor(); }
    } waitCursor;

    QProcess proc;
    proc.setProgram(pdfApp);
    proc.setArguments({});
    proc.setWorkingDirectory(buildDir);
    proc.setProcessChannelMode(QProcess::MergedChannels);

    proc.start(QIODevice::ReadOnly);
    if (!proc.waitForStarted(15000))
    {
        QMessageBox::critical(
            this,
            QStringLiteral("Generate"),
            QStringLiteral("Could not start pdf_app:\n%1").arg(proc.errorString()));
        return;
    }

    constexpr int kTimeoutMs = 600000;
    if (!proc.waitForFinished(kTimeoutMs))
    {
        proc.kill();
        proc.waitForFinished(5000);
        QMessageBox::critical(
            this,
            QStringLiteral("Generate"),
            QStringLiteral("pdf_app did not finish within %1 minutes.").arg(kTimeoutMs / 60000));
        return;
    }

    const int exitCode = proc.exitStatus() == QProcess::NormalExit ? proc.exitCode() : -1;
    const QString merged = QString::fromUtf8(proc.readAllStandardOutput());
    const QStringList errors = ExtractPdfGeneratorErrorLines(merged);
    const bool noReportedErrors = errors.isEmpty() && exitCode == 0;

    if (noReportedErrors)
    {
        QMessageBox::information(
            this,
            QStringLiteral("Generate"),
            QStringLiteral("PDF generation completed successfully with no reported errors.\n\n"
                           "PDFs were written to:\n%1")
                .arg(QDir::toNativeSeparators(buildDir)));
        return;
    }

    QString detail;
    if (exitCode != 0)
    {
        detail += QStringLiteral("Exit status: %1\n\n").arg(exitCode);
    }
    if (!errors.isEmpty())
    {
        detail += errors.join(u'\n');
    }
    else
    {
        detail += QStringLiteral("(No lines containing ERROR: or Error: were found.)\n\n");
        detail += QStringLiteral("--- Output (tail) ---\n");
        detail += merged.size() > 12000 ? merged.right(12000) : merged;
    }

    QMessageBox msg(this);
    msg.setWindowTitle(QStringLiteral("Generate"));
    msg.setIcon(QMessageBox::Warning);
    msg.setText(QStringLiteral("PDF generation reported problems. Use \"Show Details…\" for the full list."));
    msg.setInformativeText(
        QStringLiteral("Output directory:\n%1").arg(QDir::toNativeSeparators(buildDir)));
    msg.setDetailedText(detail);
    msg.exec();
}

void MainWindow::OnRefreshFileListButton() { RefreshFileList(); }

void MainWindow::OnRootStringChanged(const QString& value)
{
    QObject* s = sender();
    if (!s) { return; }
    const QByteArray key = s->property("jsonKey").toByteArray();
    if (key.isEmpty()) { return; }
    doc.Json()[key.constData()] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnRootIntChanged(int value)
{
    QObject* s = sender();
    if (!s) { return; }
    const QByteArray key = s->property("jsonKey").toByteArray();
    if (key.isEmpty()) { return; }
    doc.Json()[key.constData()] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnRootDoubleChanged(double value)
{
    QObject* s = sender();
    if (!s) { return; }
    const QByteArray key = s->property("jsonKey").toByteArray();
    if (key.isEmpty()) { return; }
    const double rounded = std::round(value * 100.0) / 100.0;
    doc.Json()[key.constData()] = rounded;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnStatIntChanged(int value)
{
    QObject* s = sender();
    if (!s) { return; }
    const QByteArray key = s->property("jsonKey").toByteArray();
    if (key.isEmpty()) { return; }
    doc.Json()["stats"][key.constData()] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnProficiencyBonusChanged(int value)
{
    doc.Json()["proficiencies"]["bonus"] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnSavingThrowChanged(int value)
{
    QObject* s = sender();
    if (!s) { return; }
    const QByteArray stat = s->property("statKey").toByteArray();
    if (stat.isEmpty()) { return; }
    doc.Json()["proficiencies"]["savingThrows"][stat.constData()] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnSkillValueChanged(int value)
{
    QObject* s = sender();
    if (!s) { return; }
    const QByteArray stat = s->property("statKey").toByteArray();
    const QByteArray skill = s->property("skillKey").toByteArray();
    if (stat.isEmpty() || skill.isEmpty()) { return; }
    doc.Json()["proficiencies"]["skills"][stat.constData()][skill.constData()] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::BuildTabs()
{
    tabs->clear();
    tabs->addTab(WrapTabInScrollArea(BuildGeneralTab()), "General");
    tabs->addTab(WrapTabInScrollArea(BuildProficienciesTab()), "Proficiencies");
    tabs->addTab(WrapTabInScrollArea(BuildClassesTab()), "Classes");
    tabs->addTab(WrapTabInScrollArea(BuildEquipmentTab()), "Equipment");
    tabs->addTab(WrapTabInScrollArea(BuildBackpackTab()), "Backpack");
    tabs->addTab(BuildRawJsonTab(), "Raw JSON");
}

static int GetIntOrDefault(const nlohmann::ordered_json& obj, const char* key, int def)
{
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_number_integer()) { return def; }
    return obj.at(key).get<int>();
}

static double GetDoubleOrDefault(const nlohmann::ordered_json& obj, const char* key, double def)
{
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_number()) { return def; }
    return obj.at(key).get<double>();
}

static std::string GetStringOrDefault(const nlohmann::ordered_json& obj, const char* key, const std::string& def)
{
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_string()) { return def; }
    return obj.at(key).get<std::string>();
}

void MainWindow::SetSpinValue(QSpinBox* spin, const nlohmann::ordered_json& obj, const char* key, int def)
{
    if (!spin) { return; }
    spin->setValue(GetIntOrDefault(obj, key, def));
}

std::vector<std::string> MainWindow::GetProficiencyArray(const std::string& key)
{
    const nlohmann::ordered_json& root = std::as_const(doc).Json();
    if (!root.contains("proficiencies") || !root["proficiencies"].is_object()) { return {}; }
    const auto& prof = root["proficiencies"];
    if (!prof.contains(key) || !prof[key].is_array()) { return {}; }
    std::vector<std::string> out;
    for (const auto& el : prof[key])
    {
        if (el.is_string()) { out.push_back(el.get<std::string>()); }
    }
    return out;
}

void MainWindow::SetProficiencyArray(const std::string& key, const std::vector<std::string>& items)
{
    auto& arr = doc.Json()["proficiencies"][key];
    arr = nlohmann::ordered_json::array();
    for (const auto& s : items) { arr.push_back(s); }
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::EnsureSavingThrowsInitialized()
{
    auto& prof = doc.Json()["proficiencies"];
    if (!prof.contains("savingThrows") || !prof["savingThrows"].is_object())
    {
        prof["savingThrows"] = nlohmann::ordered_json::object();
    }
    for (const char* stat : {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"})
    {
        if (!prof["savingThrows"].contains(stat) || !prof["savingThrows"][stat].is_number_integer())
        {
            prof["savingThrows"][stat] = 0;
        }
    }
}

void MainWindow::EnsureSkillsInitialized()
{
    auto& prof = doc.Json()["proficiencies"];
    if (!prof.contains("skills") || !prof["skills"].is_object())
    {
        prof["skills"] = nlohmann::ordered_json::object();
    }
    auto& skills = prof["skills"];
    if (!skills.contains("strength")) { skills["strength"] = nlohmann::ordered_json::object(); }
    if (!skills.contains("dexterity")) { skills["dexterity"] = nlohmann::ordered_json::object(); }
    if (!skills.contains("intelligence")) { skills["intelligence"] = nlohmann::ordered_json::object(); }
    if (!skills.contains("wisdom")) { skills["wisdom"] = nlohmann::ordered_json::object(); }
    if (!skills.contains("charisma")) { skills["charisma"] = nlohmann::ordered_json::object(); }
    if (!skills["strength"].contains("athletics")) { skills["strength"]["athletics"] = 0; }
    for (const char* k : {"acrobatics", "sleightOfHand", "stealth"})
    {
        if (!skills["dexterity"].contains(k)) { skills["dexterity"][k] = 0; }
    }
    for (const char* k : {"arcana", "history", "investigation", "nature", "religion"})
    {
        if (!skills["intelligence"].contains(k)) { skills["intelligence"][k] = 0; }
    }
    for (const char* k : {"animalHandling", "insight", "medicine", "perception", "survival"})
    {
        if (!skills["wisdom"].contains(k)) { skills["wisdom"][k] = 0; }
    }
    for (const char* k : {"deception", "intimidation", "performance", "persuasion"})
    {
        if (!skills["charisma"].contains(k)) { skills["charisma"][k] = 0; }
    }
}

QSpinBox* MainWindow::CreateSkillSpinBox(const char* statKey, const char* skillKey)
{
    QSpinBox* s = new QSpinBox();
    s->setRange(0, 50);
    const nlohmann::ordered_json& root = std::as_const(doc).Json();
    s->setValue(GetIntOrDefault(root.at("proficiencies").at("skills").at(statKey), skillKey, 0));
    s->setProperty("statKey", statKey);
    s->setProperty("skillKey", skillKey);
    QObject::connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnSkillValueChanged);
    profSkillSpins.push_back(s);
    return s;
}

QGroupBox* MainWindow::BuildSkillStatGroup(const char* title,
                                           const char* statKey,
                                           const std::vector<const char*>& skillKeys)
{
    QGroupBox* box = new QGroupBox(title);
    QFormLayout* f = new QFormLayout();
    box->setLayout(f);
    for (const char* k : skillKeys)
    {
        f->addRow(QString::fromStdString(std::string(k)), CreateSkillSpinBox(statKey, k));
    }
    return box;
}

std::vector<std::string> MainWindow::GetSelectedClassSpells()
{
    if (!classesList || !classesSpellLevel) { return {}; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return {}; }
    const std::string classId = item->text().toStdString();
    if (!doc.Json().contains("classes") || !doc.Json()["classes"].is_object()) { return {}; }
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { return {}; }
    if (!cls.contains("spells") || !cls["spells"].is_object()) { cls["spells"] = nlohmann::ordered_json::object(); }
    const std::string lvlKey = classesSpellLevel->currentText().toStdString();
    if (!cls["spells"].contains(lvlKey) || !cls["spells"][lvlKey].is_array())
    {
        cls["spells"][lvlKey] = nlohmann::ordered_json::array();
    }
    std::vector<std::string> out;
    for (const auto& el : cls["spells"][lvlKey])
    {
        if (el.is_string()) { out.push_back(el.get<std::string>()); }
    }
    return out;
}

void MainWindow::SetSelectedClassSpells(const std::vector<std::string>& items)
{
    if (!classesList || !classesSpellLevel) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string classId = item->text().toStdString();
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.contains("spells") || !cls["spells"].is_object()) { cls["spells"] = nlohmann::ordered_json::object(); }
    const std::string lvlKey = classesSpellLevel->currentText().toStdString();
    auto& arr = cls["spells"][lvlKey];
    arr = nlohmann::ordered_json::array();
    for (const auto& s : items) { arr.push_back(s); }
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::RefreshClassList()
{
    if (!classesList) { return; }
    classesList->clear();
    if (!doc.Json().contains("classes") || !doc.Json()["classes"].is_object()) { return; }
    for (auto it = doc.Json()["classes"].begin(); it != doc.Json()["classes"].end(); ++it)
    {
        classesList->addItem(QString::fromStdString(it.key()));
    }
}

void MainWindow::LoadSelectedClassIntoEditor()
{
    if (!classesList || !classIdEdit || !classLevelSpin || !classHitDiceEdit || !classSubclassEdit || !classCastStatEdit
        || !classResourceSpin || !classesSpellsEditor)
    {
        return;
    }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item)
    {
        classIdEdit->clear();
        classIdEdit->setEnabled(false);
        return;
    }
    classIdEdit->setEnabled(true);
    const std::string classId = item->text().toStdString();
    classIdEdit->blockSignals(true);
    classIdEdit->setText(QString::fromStdString(classId));
    classIdEdit->blockSignals(false);
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { cls = nlohmann::ordered_json::object(); }

    classLevelSpin->setValue(GetIntOrDefault(cls, "level", 1));
    classHitDiceEdit->setText(QString::fromStdString(GetStringOrDefault(cls, "hitDice", "d8")));
    classSubclassEdit->setText(QString::fromStdString(GetStringOrDefault(cls, "subclass", "")));
    classCastStatEdit->setText(QString::fromStdString(GetStringOrDefault(cls, "castStat", "")));
    classResourceSpin->setValue(GetIntOrDefault(cls, "resourcePoints", 0));

    if (!cls.contains("spellslots") || !cls["spellslots"].is_object())
    {
        cls["spellslots"] = nlohmann::ordered_json::object();
    }
    for (int i = 1; i <= 9; ++i)
    {
        const std::string k = std::to_string(i);
        QSpinBox* slotSpin = classSlotSpins[static_cast<size_t>(i - 1)];
        if (slotSpin) { slotSpin->setValue(GetIntOrDefault(cls["spellslots"], k.c_str(), 0)); }
    }
    classesSpellsEditor->Refresh();
}

void MainWindow::OnClassesAddClicked()
{
    if (!classesList) { return; }
    const QString id = QInputDialog::getText(this, "Add class", "Class id (e.g. cleric, bard):");
    if (id.isEmpty()) { return; }
    if (!doc.Json().contains("classes") || !doc.Json()["classes"].is_object())
    {
        doc.Json()["classes"] = nlohmann::ordered_json::object();
    }
    doc.Json()["classes"][id.toStdString()] = nlohmann::ordered_json::object();
    CharacterSchema::NormalizeInPlace(doc.Json());
    UpdateValidationSummary();
    UpdateRawJsonView();
    RefreshClassList();
    const auto items = classesList->findItems(id, Qt::MatchExactly);
    if (!items.empty()) { classesList->setCurrentItem(items.front()); }
}

void MainWindow::OnClassesRemoveClicked()
{
    if (!classesList) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string id = item->text().toStdString();
    if (doc.Json().contains("classes") && doc.Json()["classes"].is_object())
    {
        doc.Json()["classes"].erase(id);
    }
    UpdateValidationSummary();
    UpdateRawJsonView();
    RefreshClassList();
}

void MainWindow::OnClassListCurrentItemChanged(QListWidgetItem*, QListWidgetItem*) { LoadSelectedClassIntoEditor(); }

void MainWindow::OnClassIdEditingFinished()
{
    if (!classesList || !classIdEdit) { return; }
    QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }

    const std::string oldId = item->text().toStdString();
    const std::string newId = classIdEdit->text().trimmed().toStdString();

    if (newId.empty())
    {
        QMessageBox::warning(this, "Class id", "Class id cannot be empty.");
        classIdEdit->blockSignals(true);
        classIdEdit->setText(QString::fromStdString(oldId));
        classIdEdit->blockSignals(false);
        return;
    }
    if (newId == oldId) { return; }

    if (!doc.Json().contains("classes") || !doc.Json()["classes"].is_object()) { return; }
    auto& classes = doc.Json()["classes"];
    if (classes.contains(newId))
    {
        QMessageBox::warning(this, "Class id", "A class with that id already exists.");
        classIdEdit->blockSignals(true);
        classIdEdit->setText(QString::fromStdString(oldId));
        classIdEdit->blockSignals(false);
        return;
    }
    if (!classes.contains(oldId)) { return; }

    nlohmann::ordered_json rebuilt = nlohmann::ordered_json::object();
    for (auto it = classes.begin(); it != classes.end(); ++it)
    {
        if (it.key() == oldId) { rebuilt[newId] = it.value(); }
        else { rebuilt[it.key()] = it.value(); }
    }
    classes.swap(rebuilt);

    item->setText(QString::fromStdString(newId));
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnClassSpellLevelComboChanged(const QString&)
{
    if (classesSpellsEditor) { classesSpellsEditor->Refresh(); }
}

void MainWindow::OnClassLevelSpinChanged(int value)
{
    if (!classesList) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string classId = item->text().toStdString();
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { cls = nlohmann::ordered_json::object(); }
    cls["level"] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnClassResourcePointsChanged(int value)
{
    if (!classesList) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string classId = item->text().toStdString();
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { cls = nlohmann::ordered_json::object(); }
    cls["resourcePoints"] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnClassHitDiceChanged(const QString& value)
{
    if (!classesList) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string classId = item->text().toStdString();
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { cls = nlohmann::ordered_json::object(); }
    cls["hitDice"] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnClassSubclassChanged(const QString& value)
{
    if (!classesList) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string classId = item->text().toStdString();
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { cls = nlohmann::ordered_json::object(); }
    cls["subclass"] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnClassCastStatChanged(const QString& value)
{
    if (!classesList) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string classId = item->text().toStdString();
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { cls = nlohmann::ordered_json::object(); }
    cls["castStat"] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnClassSpellSlotChanged(int value)
{
    QObject* s = sender();
    if (!s || !classesList) { return; }
    bool ok = false;
    const int slotLevel = s->property("slotLevel").toInt(&ok);
    if (!ok || slotLevel < 1 || slotLevel > 9) { return; }
    const QListWidgetItem* item = classesList->currentItem();
    if (!item) { return; }
    const std::string classId = item->text().toStdString();
    auto& cls = doc.Json()["classes"][classId];
    if (!cls.is_object()) { cls = nlohmann::ordered_json::object(); }
    if (!cls.contains("spellslots") || !cls["spellslots"].is_object())
    {
        cls["spellslots"] = nlohmann::ordered_json::object();
    }
    cls["spellslots"][std::to_string(slotLevel)] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::EnsureEquipmentStructure() { CharacterSchema::NormalizeInPlace(doc.Json()); }

nlohmann::ordered_json* MainWindow::CurrentEquipmentArray()
{
    EnsureEquipmentStructure();
    if (!equipBucket || !equipKind) { return nullptr; }
    const std::string b = equipBucket->currentText().toStdString();
    const std::string k = equipKind->currentText().toStdString();
    auto& arr = doc.Json()["equipment"][b][k];
    if (!arr.is_array()) { arr = nlohmann::ordered_json::array(); }
    return &arr;
}

nlohmann::ordered_json* MainWindow::CurrentEquipmentItem()
{
    nlohmann::ordered_json* arr = CurrentEquipmentArray();
    if (!arr || !equipItems) { return nullptr; }
    const int row = equipItems->currentRow();
    if (row < 0 || static_cast<size_t>(row) >= arr->size()) { return nullptr; }
    auto& obj = (*arr)[static_cast<size_t>(row)];
    if (!obj.is_object()) { obj = nlohmann::ordered_json::object(); }
    return &obj;
}

void MainWindow::RefreshEquipmentItemList()
{
    if (!equipItems) { return; }
    equipItems->clear();
    nlohmann::ordered_json* arr = CurrentEquipmentArray();
    if (!arr) { return; }
    for (size_t i = 0; i < arr->size(); ++i)
    {
        const auto& obj = (*arr)[i];
        std::string label = std::string("#") + std::to_string(i);
        if (obj.is_object() && obj.contains("name") && obj["name"].is_string())
        {
            label += " ";
            label += obj["name"].get<std::string>();
        }
        equipItems->addItem(QString::fromStdString(label));
    }
}

void MainWindow::LoadSelectedEquipmentIntoEditor()
{
    if (!equipItems || !equipKind || !equipName || !equipType || !equipExtraText) { return; }

    nlohmann::ordered_json* arr = CurrentEquipmentArray();
    const int row = equipItems->currentRow();
    const std::string k = equipKind->currentText().toStdString();
    const bool isWeapon = (k == "weapons");
    if (equipWeaponBox) { equipWeaponBox->setVisible(isWeapon); }
    if (equipDmgBaseGroup) { equipDmgBaseGroup->setVisible(isWeapon); }
    if (equipDmgAltGroup) { equipDmgAltGroup->setVisible(isWeapon); }
    if (equipDmgExtra) { equipDmgExtra->setVisible(isWeapon); }
    if (equipRange) { equipRange->setVisible(isWeapon); }
    if (equipArmorBox) { equipArmorBox->setVisible(!isWeapon); }

    if (!arr || row < 0 || static_cast<size_t>(row) >= arr->size())
    {
        equipName->setText("");
        equipType->setText("");
        equipExtraText->setText("");
        return;
    }

    auto& obj = (*arr)[static_cast<size_t>(row)];
    if (!obj.is_object()) { obj = nlohmann::ordered_json::object(); }

    equipName->setText(QString::fromStdString(GetStringOrDefault(obj, "name", "")));
    equipType->setText(QString::fromStdString(GetStringOrDefault(obj, "type", "")));
    equipExtraText->setText(QString::fromStdString(GetStringOrDefault(obj, "extratext", "")));

    if (isWeapon)
    {
        if (!obj.contains("props") || !obj["props"].is_array()) { obj["props"] = nlohmann::ordered_json::array(); }
        if (!obj.contains("damage") || !obj["damage"].is_object()) { obj["damage"] = nlohmann::ordered_json::object(); }
        if (!obj["damage"].contains("base") || !obj["damage"]["base"].is_object())
        {
            obj["damage"]["base"] = nlohmann::ordered_json::object();
        }
        if (!obj["damage"].contains("extra") || !obj["damage"]["extra"].is_array())
        {
            obj["damage"]["extra"] = nlohmann::ordered_json::array();
        }
        if (!obj["damage"].contains("alt") || !obj["damage"]["alt"].is_object())
        {
            obj["damage"]["alt"] = nlohmann::ordered_json::object();
        }
        if (equipRange) { equipRange->setText(QString::fromStdString(GetStringOrDefault(obj, "range", ""))); }
        auto& base = obj["damage"]["base"];
        if (equipDmgDice) { equipDmgDice->setText(QString::fromStdString(GetStringOrDefault(base, "dice", ""))); }
        if (equipDmgBonus) { equipDmgBonus->setValue(GetIntOrDefault(base, "bonus", 0)); }
        if (equipDmgType) { equipDmgType->setText(QString::fromStdString(GetStringOrDefault(base, "type", ""))); }
        auto& alt = obj["damage"]["alt"];
        if (equipAltDmgDice) { equipAltDmgDice->setText(QString::fromStdString(GetStringOrDefault(alt, "dice", ""))); }
        if (equipAltDmgBonus) { equipAltDmgBonus->setValue(GetIntOrDefault(alt, "bonus", 0)); }
        if (equipAltDmgType) { equipAltDmgType->setText(QString::fromStdString(GetStringOrDefault(alt, "type", ""))); }
        if (equipWeaponProps) { equipWeaponProps->Refresh(); }
        if (equipDmgExtra) { equipDmgExtra->Refresh(); }
    }
    else
    {
        if (!obj.contains("ac") || !obj["ac"].is_object()) { obj["ac"] = nlohmann::ordered_json::object(); }
        const bool statMode = obj["ac"].contains("base");
        if (equipAcModeStat && equipAcModeFix)
        {
            QSignalBlocker blockStat(equipAcModeStat);
            QSignalBlocker blockFix(equipAcModeFix);
            equipAcModeStat->setChecked(statMode);
            equipAcModeFix->setChecked(!statMode);
        }
        if (equipArmorAcStack) { equipArmorAcStack->setCurrentIndex(statMode ? 0 : 1); }
        if (equipAcBase) { equipAcBase->setValue(GetIntOrDefault(obj["ac"], "base", 14)); }
        if (equipAcModStat) { equipAcModStat->setText(QString::fromStdString(GetStringOrDefault(obj["ac"], "modstat", "dexterity"))); }
        if (equipAcModCap) { equipAcModCap->setValue(GetIntOrDefault(obj["ac"], "modcap", 0)); }
        if (equipAcFixMod) { equipAcFixMod->setValue(GetIntOrDefault(obj["ac"], "fixmod", 2)); }
    }
}

void MainWindow::OnEquipmentBucketOrKindChanged(const QString&)
{
    RefreshEquipmentItemList();
    LoadSelectedEquipmentIntoEditor();
}

void MainWindow::OnEquipmentRowChanged(int) { LoadSelectedEquipmentIntoEditor(); }

void MainWindow::OnEquipmentAddClicked()
{
    if (!equipKind || !equipItems) { return; }
    nlohmann::ordered_json* arr = CurrentEquipmentArray();
    if (!arr) { return; }
    const std::string k = equipKind->currentText().toStdString();
    nlohmann::ordered_json obj = nlohmann::ordered_json::object();
    obj["name"] = (k == "weapons") ? "New Weapon" : "New Armor";
    obj["type"] = "";
    obj["extratext"] = "";
    if (k == "armors")
    {
        obj["ac"] = nlohmann::ordered_json::object({{"base", 0}});
    }
    else
    {
        obj["props"] = nlohmann::ordered_json::array();
        obj["damage"] = nlohmann::ordered_json::object(
            {{"base", nlohmann::ordered_json::object({{"dice", "d4"}, {"bonus", 0}, {"type", ""}})},
             {"alt", nlohmann::ordered_json::object()},
             {"extra", nlohmann::ordered_json::array()}});
    }
    arr->push_back(obj);
    UpdateValidationSummary();
    UpdateRawJsonView();
    RefreshEquipmentItemList();
    equipItems->setCurrentRow(static_cast<int>(arr->size() - 1));
}

void MainWindow::OnEquipmentRemoveClicked()
{
    if (!equipItems) { return; }
    nlohmann::ordered_json* arr = CurrentEquipmentArray();
    const int row = equipItems->currentRow();
    if (!arr || row < 0 || static_cast<size_t>(row) >= arr->size()) { return; }
    arr->erase(arr->begin() + row);
    UpdateValidationSummary();
    UpdateRawJsonView();
    RefreshEquipmentItemList();
}

void MainWindow::OnEquipmentCommonFieldChanged(const QString& value)
{
    QObject* s = sender();
    if (!s) { return; }
    const QByteArray key = s->property("equipField").toByteArray();
    if (key.isEmpty()) { return; }
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj) { return; }
    (*obj)[key.constData()] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAcBaseChanged(int value)
{
    if (!equipAcModeStat || !equipAcModeStat->isChecked()) { return; }
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj) { return; }
    if (!obj->contains("ac") || !(*obj)["ac"].is_object()) { (*obj)["ac"] = nlohmann::ordered_json::object(); }
    (*obj)["ac"]["base"] = value;
    (*obj)["ac"].erase("fixmod");
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAcModStatChanged(const QString& value)
{
    if (!equipAcModeStat || !equipAcModeStat->isChecked()) { return; }
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj) { return; }
    if (!obj->contains("ac") || !(*obj)["ac"].is_object()) { (*obj)["ac"] = nlohmann::ordered_json::object(); }
    (*obj)["ac"]["modstat"] = value.toStdString();
    (*obj)["ac"].erase("fixmod");
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAcModCapChanged(int value)
{
    if (!equipAcModeStat || !equipAcModeStat->isChecked()) { return; }
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj) { return; }
    if (!obj->contains("ac") || !(*obj)["ac"].is_object()) { (*obj)["ac"] = nlohmann::ordered_json::object(); }
    (*obj)["ac"]["modcap"] = value;
    (*obj)["ac"].erase("fixmod");
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAcFixModChanged(int value)
{
    if (!equipAcModeFix || !equipAcModeFix->isChecked()) { return; }
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj) { return; }
    if (!obj->contains("ac") || !(*obj)["ac"].is_object()) { (*obj)["ac"] = nlohmann::ordered_json::object(); }
    (*obj)["ac"]["fixmod"] = value;
    (*obj)["ac"].erase("base");
    (*obj)["ac"].erase("modstat");
    (*obj)["ac"].erase("modcap");
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::ApplyArmorAcModeToDocument(bool statBased)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "armors") { return; }

    if (statBased)
    {
        int b = equipAcBase ? equipAcBase->value() : 14;
        if (b < 1) { b = 14; }
        std::string ms = equipAcModStat ? equipAcModStat->text().toStdString() : "";
        if (ms.empty()) { ms = "dexterity"; }
        int mc = equipAcModCap ? equipAcModCap->value() : 0;
        if (mc < 0) { mc = 0; }
        (*obj)["ac"] = nlohmann::ordered_json({{"base", b}, {"modstat", ms}, {"modcap", mc}});
        if (equipArmorAcStack) { equipArmorAcStack->setCurrentIndex(0); }
        if (equipAcBase)
        {
            QSignalBlocker blockBase(equipAcBase);
            equipAcBase->setValue(b);
        }
        if (equipAcModStat)
        {
            QSignalBlocker blockMs(equipAcModStat);
            equipAcModStat->setText(QString::fromStdString(ms));
        }
        if (equipAcModCap)
        {
            QSignalBlocker blockMc(equipAcModCap);
            equipAcModCap->setValue(mc);
        }
    }
    else
    {
        int fm = equipAcFixMod ? equipAcFixMod->value() : 2;
        if (fm < 1) { fm = 2; }
        (*obj)["ac"] = nlohmann::ordered_json({{"fixmod", fm}});
        if (equipArmorAcStack) { equipArmorAcStack->setCurrentIndex(1); }
        if (equipAcFixMod)
        {
            QSignalBlocker blockFm(equipAcFixMod);
            equipAcFixMod->setValue(fm);
        }
    }
}

void MainWindow::OnEquipmentAcModeStatToggled(bool checked)
{
    if (!checked) { return; }
    ApplyArmorAcModeToDocument(true);
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAcModeFixToggled(bool checked)
{
    if (!checked) { return; }
    ApplyArmorAcModeToDocument(false);
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentRangeChanged(const QString& value)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    const std::string s = value.toStdString();
    if (s.empty())
    {
        obj->erase("range");
    }
    else
    {
        (*obj)["range"] = s;
    }
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentDmgDiceChanged(const QString& value)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    if (!(*obj)["damage"].contains("base") || !(*obj)["damage"]["base"].is_object())
    {
        (*obj)["damage"]["base"] = nlohmann::ordered_json::object();
    }
    (*obj)["damage"]["base"]["dice"] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentDmgBonusChanged(int value)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    if (!(*obj)["damage"].contains("base") || !(*obj)["damage"]["base"].is_object())
    {
        (*obj)["damage"]["base"] = nlohmann::ordered_json::object();
    }
    (*obj)["damage"]["base"]["bonus"] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentDmgTypeChanged(const QString& value)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    if (!(*obj)["damage"].contains("base") || !(*obj)["damage"]["base"].is_object())
    {
        (*obj)["damage"]["base"] = nlohmann::ordered_json::object();
    }
    (*obj)["damage"]["base"]["type"] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAltDmgDiceChanged(const QString& value)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    if (!(*obj)["damage"].contains("alt") || !(*obj)["damage"]["alt"].is_object())
    {
        (*obj)["damage"]["alt"] = nlohmann::ordered_json::object();
    }
    (*obj)["damage"]["alt"]["dice"] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAltDmgBonusChanged(int value)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    if (!(*obj)["damage"].contains("alt") || !(*obj)["damage"]["alt"].is_object())
    {
        (*obj)["damage"]["alt"] = nlohmann::ordered_json::object();
    }
    (*obj)["damage"]["alt"]["bonus"] = value;
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::OnEquipmentAltDmgTypeChanged(const QString& value)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    if (!(*obj)["damage"].contains("alt") || !(*obj)["damage"]["alt"].is_object())
    {
        (*obj)["damage"]["alt"] = nlohmann::ordered_json::object();
    }
    (*obj)["damage"]["alt"]["type"] = value.toStdString();
    UpdateValidationSummary();
    UpdateRawJsonView();
}

std::vector<std::string> MainWindow::GetEquipmentWeaponProps()
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return {}; }
    if (!obj->contains("props") || !(*obj)["props"].is_array()) { (*obj)["props"] = nlohmann::ordered_json::array(); }
    std::vector<std::string> out;
    for (const auto& el : (*obj)["props"])
    {
        if (el.is_string()) { out.push_back(el.get<std::string>()); }
    }
    return out;
}

void MainWindow::SetEquipmentWeaponProps(const std::vector<std::string>& items)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    auto& arr = (*obj)["props"];
    arr = nlohmann::ordered_json::array();
    for (const auto& s : items) { arr.push_back(s); }
    UpdateValidationSummary();
    UpdateRawJsonView();
}

std::vector<std::string> MainWindow::GetEquipmentDamageExtraStrings()
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return {}; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    if (!(*obj)["damage"].contains("extra") || !(*obj)["damage"]["extra"].is_array())
    {
        (*obj)["damage"]["extra"] = nlohmann::ordered_json::array();
    }
    std::vector<std::string> out;
    for (const auto& el : (*obj)["damage"]["extra"])
    {
        if (el.is_string()) { out.push_back(el.get<std::string>()); }
    }
    return out;
}

void MainWindow::SetEquipmentDamageExtraStrings(const std::vector<std::string>& items)
{
    nlohmann::ordered_json* obj = CurrentEquipmentItem();
    if (!obj || !equipKind || equipKind->currentText().toStdString() != "weapons") { return; }
    if (!obj->contains("damage") || !(*obj)["damage"].is_object()) { (*obj)["damage"] = nlohmann::ordered_json::object(); }
    auto& arr = (*obj)["damage"]["extra"];
    arr = nlohmann::ordered_json::array();
    for (const auto& s : items) { arr.push_back(s); }
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::EnsureBackpackObject()
{
    if (!doc.Json().contains("backpack") || !doc.Json()["backpack"].is_object())
    {
        doc.Json()["backpack"] = nlohmann::ordered_json::object();
    }
    auto& backpack = doc.Json()["backpack"];
    for (const char* key : kBackpackSectionKeys)
    {
        if (!backpack.contains(key) || !backpack[key].is_array()) { backpack[key] = nlohmann::ordered_json::array(); }
    }
}

std::vector<std::string> MainWindow::GetBackpackSectionItemsForKey(const char* sectionKey)
{
    EnsureBackpackObject();
    if (!sectionKey) { return {}; }
    auto& backpack = doc.Json()["backpack"];
    if (!backpack.contains(sectionKey) || !backpack[sectionKey].is_array())
    {
        backpack[sectionKey] = nlohmann::ordered_json::array();
    }
    std::vector<std::string> out;
    for (const auto& el : backpack[sectionKey])
    {
        if (el.is_string()) { out.push_back(el.get<std::string>()); }
    }
    return out;
}

void MainWindow::SetBackpackSectionItemsForKey(const char* sectionKey, const std::vector<std::string>& items)
{
    EnsureBackpackObject();
    if (!sectionKey) { return; }
    auto& arr = doc.Json()["backpack"][sectionKey];
    arr = nlohmann::ordered_json::array();
    for (const auto& s : items) { arr.push_back(s); }
    UpdateValidationSummary();
    UpdateRawJsonView();
}

void MainWindow::RefreshBackpackSections()
{
    EnsureBackpackObject();
    for (StringListEditor* ed : backpackSectionEditors)
    {
        if (ed) { ed->Refresh(); }
    }
}

std::vector<std::string> MainWindow::GetTraits()
{
    const nlohmann::ordered_json& root = std::as_const(doc).Json();
    if (!root.contains("traits") || !root["traits"].is_array()) { return {}; }
    std::vector<std::string> out;
    for (const auto& el : root["traits"])
    {
        if (el.is_string()) { out.push_back(el.get<std::string>()); }
    }
    return out;
}

void MainWindow::SetTraits(const std::vector<std::string>& items)
{
    auto& arr = doc.Json()["traits"];
    arr = nlohmann::ordered_json::array();
    for (const auto& s : items) { arr.push_back(s); }
    UpdateValidationSummary();
    UpdateRawJsonView();
}

QWidget* MainWindow::BuildGeneralTab()
{
    QWidget* w = new QWidget();
    generalTab = w;
    QVBoxLayout* outer = new QVBoxLayout(w);
    QHBoxLayout* root = new QHBoxLayout();
    outer->addLayout(root);

    QGroupBox* generalBox = new QGroupBox("Character");
    QFormLayout* generalForm = new QFormLayout();
    generalBox->setLayout(generalForm);
    root->addWidget(generalBox, 1);

    QGroupBox* statsBox = new QGroupBox("Stats");
    QFormLayout* statsForm = new QFormLayout();
    statsBox->setLayout(statsForm);
    root->addWidget(statsBox, 1);

    generalName = new QLineEdit();
    generalName->setProperty("jsonKey", "name");
    generalNameSpacing = new QDoubleSpinBox();
    generalNameSpacing->setRange(-50.0, 50.0);
    generalNameSpacing->setDecimals(2);
    generalNameSpacing->setSingleStep(0.1);
    generalNameSpacing->setProperty("jsonKey", "nameLetterSpacing");
    generalRace = new QLineEdit();
    generalRace->setProperty("jsonKey", "race");
    generalBackground = new QLineEdit();
    generalBackground->setProperty("jsonKey", "background");

    generalForm->addRow("Name", generalName);
    generalForm->addRow("Name letter spacing", generalNameSpacing);
    generalForm->addRow("Race", generalRace);
    generalForm->addRow("Background", generalBackground);

    QObject::connect(generalName, &QLineEdit::textChanged, this, &MainWindow::OnRootStringChanged);
    QObject::connect(generalRace, &QLineEdit::textChanged, this, &MainWindow::OnRootStringChanged);
    QObject::connect(generalBackground, &QLineEdit::textChanged, this, &MainWindow::OnRootStringChanged);
    QObject::connect(generalNameSpacing,
                     QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     this,
                     &MainWindow::OnRootDoubleChanged);

    statStrength = MakeSpinBox(0, 30);
    statStrength->setProperty("jsonKey", "strength");
    statDexterity = MakeSpinBox(0, 30);
    statDexterity->setProperty("jsonKey", "dexterity");
    statConstitution = MakeSpinBox(0, 30);
    statConstitution->setProperty("jsonKey", "constitution");
    statIntelligence = MakeSpinBox(0, 30);
    statIntelligence->setProperty("jsonKey", "intelligence");
    statWisdom = MakeSpinBox(0, 30);
    statWisdom->setProperty("jsonKey", "wisdom");
    statCharisma = MakeSpinBox(0, 30);
    statCharisma->setProperty("jsonKey", "charisma");
    statSpeed = MakeSpinBox(1, 200);
    statSpeed->setProperty("jsonKey", "speed");
    statMaxHp = MakeSpinBox(1, 999);
    statMaxHp->setProperty("jsonKey", "maxHp");
    statAc = MakeSpinBox(1, 99);
    statAc->setProperty("jsonKey", "ac");
    statInitiativeBonus = MakeSpinBox(0, 50);
    statInitiativeBonus->setProperty("jsonKey", "initiativeBonus");
    statPPercBonus = MakeSpinBox(0, 50);
    statPPercBonus->setProperty("jsonKey", "pPercBonus");

    statsForm->addRow("Strength", statStrength);
    statsForm->addRow("Dexterity", statDexterity);
    statsForm->addRow("Constitution", statConstitution);
    statsForm->addRow("Intelligence", statIntelligence);
    statsForm->addRow("Wisdom", statWisdom);
    statsForm->addRow("Charisma", statCharisma);
    statsForm->addRow("Speed", statSpeed);
    statsForm->addRow("Max HP", statMaxHp);
    statsForm->addRow("AC", statAc);
    statsForm->addRow("Initiative bonus", statInitiativeBonus);
    statsForm->addRow("Passive perception bonus", statPPercBonus);

    QObject::connect(statStrength, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statDexterity, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statConstitution, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statIntelligence, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statWisdom, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statCharisma, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statSpeed, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statMaxHp, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statAc, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);
    QObject::connect(statInitiativeBonus,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     this,
                     &MainWindow::OnStatIntChanged);
    QObject::connect(statPPercBonus, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnStatIntChanged);

    QGroupBox* traitsBox = new QGroupBox("Traits");
    outer->addWidget(traitsBox, 1);
    QVBoxLayout* traitsLayout = new QVBoxLayout();
    traitsBox->setLayout(traitsLayout);
    QLabel* traitsHint =
        new QLabel("Trait names as in assets/cfg/config_traits.json (Racial Traits, Class Features, etc.).");
    traitsHint->setWordWrap(true);
    traitsLayout->addWidget(traitsHint);
    generalTraitsEditor =
        new StringListEditor(std::bind(&MainWindow::GetTraits, this), std::bind(&MainWindow::SetTraits, this, std::placeholders::_1));
    traitsLayout->addWidget(generalTraitsEditor, 1);
    generalTraitsEditor->Refresh();

    return w;
}

QWidget* MainWindow::BuildProficienciesTab()
{
    QWidget* w = new QWidget();
    profTab = w;
    profSavingThrowSpins = {};
    profSkillSpins.clear();
    profLanguagesEditor = nullptr;
    profToolsEditor = nullptr;
    profArmorsEditor = nullptr;
    profSimpleWeaponsEditor = nullptr;
    profMartialWeaponsEditor = nullptr;

    QVBoxLayout* layout = new QVBoxLayout();
    w->setLayout(layout);

    QFormLayout* form = new QFormLayout();
    layout->addLayout(form);

    profBonusSpin = new QSpinBox();
    profBonusSpin->setRange(1, 20);
    form->addRow("Proficiency bonus", profBonusSpin);
    QObject::connect(profBonusSpin,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     this,
                     &MainWindow::OnProficiencyBonusChanged);

    QGroupBox* listsGroup = new QGroupBox("Languages, tools & weapon/armor training");
    layout->addWidget(listsGroup);
    QGridLayout* listsGrid = new QGridLayout();
    listsGroup->setLayout(listsGrid);

    auto makeListCell = [](const char* title, StringListEditor* ed) {
        QWidget* cell = new QWidget();
        QVBoxLayout* vl = new QVBoxLayout(cell);
        vl->setContentsMargins(0, 0, 0, 0);
        QLabel* lab = new QLabel(QString::fromUtf8(title));
        vl->addWidget(lab);
        vl->addWidget(ed, 1);
        return cell;
    };

    {
        const std::string k = "languages";
        profLanguagesEditor = new StringListEditor(std::bind(&MainWindow::GetProficiencyArray, this, k),
                                                   std::bind(&MainWindow::SetProficiencyArray,
                                                             this,
                                                             k,
                                                             std::placeholders::_1));
        profLanguagesEditor->Refresh();
        listsGrid->addWidget(makeListCell("Languages", profLanguagesEditor), 0, 0);
    }
    {
        const std::string k = "tools";
        profToolsEditor = new StringListEditor(std::bind(&MainWindow::GetProficiencyArray, this, k),
                                               std::bind(&MainWindow::SetProficiencyArray,
                                                         this,
                                                         k,
                                                         std::placeholders::_1));
        profToolsEditor->Refresh();
        listsGrid->addWidget(makeListCell("Tools", profToolsEditor), 0, 1);
    }
    {
        const std::string k = "simpleWeapons";
        profSimpleWeaponsEditor = new StringListEditor(std::bind(&MainWindow::GetProficiencyArray, this, k),
                                                       std::bind(&MainWindow::SetProficiencyArray,
                                                                 this,
                                                                 k,
                                                                 std::placeholders::_1));
        profSimpleWeaponsEditor->Refresh();
        listsGrid->addWidget(makeListCell("Simple weapons", profSimpleWeaponsEditor), 1, 0);
    }
    {
        const std::string k = "martialWeapons";
        profMartialWeaponsEditor = new StringListEditor(std::bind(&MainWindow::GetProficiencyArray, this, k),
                                                        std::bind(&MainWindow::SetProficiencyArray,
                                                                  this,
                                                                  k,
                                                                  std::placeholders::_1));
        profMartialWeaponsEditor->Refresh();
        listsGrid->addWidget(makeListCell("Martial weapons", profMartialWeaponsEditor), 1, 1);
    }
    {
        const std::string k = "armors";
        profArmorsEditor = new StringListEditor(std::bind(&MainWindow::GetProficiencyArray, this, k),
                                                std::bind(&MainWindow::SetProficiencyArray,
                                                          this,
                                                          k,
                                                          std::placeholders::_1));
        profArmorsEditor->Refresh();
        listsGrid->addWidget(makeListCell("Armors", profArmorsEditor), 2, 0);
    }

    QWidget* savingThrowsCell = new QWidget();
    QVBoxLayout* savingThrowsOuter = new QVBoxLayout(savingThrowsCell);
    savingThrowsOuter->setContentsMargins(0, 0, 0, 0);
    QLabel* savingThrowsTitle = new QLabel("Saving throws");
    savingThrowsOuter->addWidget(savingThrowsTitle);
    QGridLayout* saveGrid = new QGridLayout();
    savingThrowsOuter->addLayout(saveGrid);
    savingThrowsOuter->addStretch(1);

    EnsureSavingThrowsInitialized();
    static const char* savingStatKeys[] = {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"};
    static const char* savingStatLabels[] = {"Str", "Dex", "Con", "Int", "Wis", "Cha"};
    size_t saveIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        QLabel* lbl = new QLabel(QString::fromUtf8(savingStatLabels[i]));
        QSpinBox* s = new QSpinBox();
        s->setRange(0, 50);
        s->setValue(GetIntOrDefault(std::as_const(doc).Json().at("proficiencies").at("savingThrows"),
                                    savingStatKeys[i],
                                    0));
        s->setProperty("statKey", savingStatKeys[i]);
        QObject::connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnSavingThrowChanged);
        const int row = i / 3;
        const int pair = i % 3;
        saveGrid->addWidget(lbl, row, pair * 2);
        saveGrid->addWidget(s, row, pair * 2 + 1);
        if (saveIdx < profSavingThrowSpins.size()) { profSavingThrowSpins[saveIdx] = s; }
        ++saveIdx;
    }
    listsGrid->addWidget(savingThrowsCell, 2, 1);

    listsGrid->setColumnStretch(0, 1);
    listsGrid->setColumnStretch(1, 1);
    listsGrid->setRowStretch(0, 1);
    listsGrid->setRowStretch(1, 1);
    listsGrid->setRowStretch(2, 1);

    QGroupBox* skillsGroup = new QGroupBox("Skills");
    layout->addWidget(skillsGroup, 1);
    QVBoxLayout* skillsOuter = new QVBoxLayout();
    skillsGroup->setLayout(skillsOuter);

    EnsureSkillsInitialized();
    auto* skillsGridHost = new ProficiencySkillsGrid();
    skillsOuter->addWidget(skillsGridHost, 1);
    skillsGridHost->setStatGroups({BuildSkillStatGroup("Strength", "strength", {"athletics"}),
                                   BuildSkillStatGroup("Dexterity", "dexterity", {"acrobatics", "sleightOfHand", "stealth"}),
                                   BuildSkillStatGroup(
                                       "Intelligence", "intelligence", {"arcana", "history", "investigation", "nature", "religion"}),
                                   BuildSkillStatGroup(
                                       "Wisdom", "wisdom", {"animalHandling", "insight", "medicine", "perception", "survival"}),
                                   BuildSkillStatGroup(
                                       "Charisma", "charisma", {"deception", "intimidation", "performance", "persuasion"})});

    return w;
}

QWidget* MainWindow::BuildClassesTab()
{
    QWidget* w = new QWidget();
    QHBoxLayout* root = new QHBoxLayout();
    w->setLayout(root);

    QWidget* left = new QWidget();
    left->setFixedWidth(static_cast<int>(250 * 0.4));
    QVBoxLayout* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);
    root->addWidget(left, 0);

    classesList = new QListWidget();
    classesList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftLayout->addWidget(classesList, 1);

    QPushButton* addBtn = new QPushButton("Add");
    addBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    leftLayout->addWidget(addBtn);
    QPushButton* removeBtn = new QPushButton("Remove");
    removeBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    leftLayout->addWidget(removeBtn);

    QWidget* right = new QWidget();
    root->addWidget(right, 1);
    QVBoxLayout* rightLayout = new QVBoxLayout();
    right->setLayout(rightLayout);

    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    rightLayout->addWidget(scroll, 1);

    QWidget* editor = new QWidget();
    scroll->setWidget(editor);
    QVBoxLayout* editorLayout = new QVBoxLayout();
    editor->setLayout(editorLayout);

    QFormLayout* classIdForm = new QFormLayout();
    classIdEdit = new QLineEdit();
    classIdEdit->setPlaceholderText("JSON object key (e.g. cleric)");
    classIdForm->addRow("Class id", classIdEdit);
    editorLayout->addLayout(classIdForm);

    QGroupBox* basicsGroup = new QGroupBox("Basics");
    editorLayout->addWidget(basicsGroup);
    QFormLayout* basicsForm = new QFormLayout();
    basicsGroup->setLayout(basicsForm);

    classLevelSpin = new QSpinBox();
    classLevelSpin->setRange(1, 20);
    classHitDiceEdit = new QLineEdit();
    classSubclassEdit = new QLineEdit();
    classCastStatEdit = new QLineEdit();
    classResourceSpin = new QSpinBox();
    classResourceSpin->setRange(0, 999);

    basicsForm->addRow("Level", classLevelSpin);
    basicsForm->addRow("Hit dice (e.g. d8)", classHitDiceEdit);
    basicsForm->addRow("Resource points", classResourceSpin);
    basicsForm->addRow("Cast stat", classCastStatEdit);
    basicsForm->addRow("Subclass", classSubclassEdit);

    QGroupBox* slotsGroup = new QGroupBox("Spell slots (1-9)");
    editorLayout->addWidget(slotsGroup);
    QGridLayout* slotsGrid = new QGridLayout();
    slotsGroup->setLayout(slotsGrid);

    classSlotSpins = {};
    for (int i = 1; i <= 9; ++i)
    {
        QLabel* lbl = new QLabel(QString("Level %1").arg(i));
        QSpinBox* s = new QSpinBox();
        s->setRange(0, 20);
        s->setProperty("slotLevel", i);
        classSlotSpins[static_cast<size_t>(i - 1)] = s;
        slotsGrid->addWidget(lbl, (i - 1) / 3, ((i - 1) % 3) * 2);
        slotsGrid->addWidget(s, (i - 1) / 3, ((i - 1) % 3) * 2 + 1);
    }

    QGroupBox* spellsGroup = new QGroupBox("Spells");
    editorLayout->addWidget(spellsGroup, 1);
    QVBoxLayout* spellsLayout = new QVBoxLayout();
    spellsGroup->setLayout(spellsLayout);

    classesSpellLevel = new QComboBox();
    for (int i = 0; i <= 9; ++i) { classesSpellLevel->addItem(QString::number(i)); }
    classesSpellLevel->addItem("0_extra");
    spellsLayout->addWidget(new QLabel("Spell list level:"));
    spellsLayout->addWidget(classesSpellLevel);

    classesSpellsEditor = new StringListEditor(std::bind(&MainWindow::GetSelectedClassSpells, this),
                                               std::bind(&MainWindow::SetSelectedClassSpells,
                                                         this,
                                                         std::placeholders::_1));
    spellsLayout->addWidget(classesSpellsEditor, 1);

    QObject::connect(addBtn, &QPushButton::clicked, this, &MainWindow::OnClassesAddClicked);
    QObject::connect(removeBtn, &QPushButton::clicked, this, &MainWindow::OnClassesRemoveClicked);
    QObject::connect(classesList, &QListWidget::currentItemChanged, this, &MainWindow::OnClassListCurrentItemChanged);
    QObject::connect(classIdEdit, &QLineEdit::editingFinished, this, &MainWindow::OnClassIdEditingFinished);
    QObject::connect(classesSpellLevel, &QComboBox::currentTextChanged, this, &MainWindow::OnClassSpellLevelComboChanged);
    QObject::connect(classLevelSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnClassLevelSpinChanged);
    QObject::connect(classResourceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnClassResourcePointsChanged);
    QObject::connect(classHitDiceEdit, &QLineEdit::textChanged, this, &MainWindow::OnClassHitDiceChanged);
    QObject::connect(classSubclassEdit, &QLineEdit::textChanged, this, &MainWindow::OnClassSubclassChanged);
    QObject::connect(classCastStatEdit, &QLineEdit::textChanged, this, &MainWindow::OnClassCastStatChanged);

    for (QSpinBox* s : classSlotSpins)
    {
        if (s)
        {
            QObject::connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnClassSpellSlotChanged);
        }
    }

    RefreshClassList();
    return w;
}

QWidget* MainWindow::BuildEquipmentTab()
{
    QWidget* w = new QWidget();
    QHBoxLayout* root = new QHBoxLayout();
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);
    w->setLayout(root);

    QWidget* left = new QWidget();
    left->setMaximumWidth(static_cast<int>(220 * 0.7));
    root->addWidget(left, 0);
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setContentsMargins(0, 0, 0, 0);
    left->setLayout(leftLayout);

    equipBucket = new QComboBox();
    equipBucket->addItem("used");
    equipBucket->addItem("stashed");
    leftLayout->addWidget(new QLabel("Bucket:"));
    leftLayout->addWidget(equipBucket);

    equipKind = new QComboBox();
    equipKind->addItem("weapons");
    equipKind->addItem("armors");
    leftLayout->addWidget(new QLabel("Kind:"));
    leftLayout->addWidget(equipKind);

    equipItems = new QListWidget();
    leftLayout->addWidget(equipItems, 1);

    QHBoxLayout* itemButtons = new QHBoxLayout();
    leftLayout->addLayout(itemButtons);
    QPushButton* addItem = new QPushButton("Add");
    QPushButton* removeItem = new QPushButton("Remove");
    itemButtons->addWidget(addItem);
    itemButtons->addWidget(removeItem);

    QWidget* right = new QWidget();
    root->addWidget(right, 1);
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    right->setLayout(rightLayout);

    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    rightLayout->addWidget(scroll, 1);

    QWidget* editor = new QWidget();
    scroll->setWidget(editor);
    QVBoxLayout* editorLayout = new QVBoxLayout();
    editorLayout->setContentsMargins(2, 2, 2, 2);
    editorLayout->setSpacing(6);
    editor->setLayout(editorLayout);

    QGroupBox* common = new QGroupBox("Common");
    editorLayout->addWidget(common);
    QFormLayout* commonForm = new QFormLayout();
    common->setLayout(commonForm);

    equipName = new QLineEdit();
    equipName->setProperty("equipField", "name");
    equipType = new QLineEdit();
    equipType->setProperty("equipField", "type");
    equipExtraText = new QLineEdit();
    equipExtraText->setProperty("equipField", "extratext");
    commonForm->addRow("Name", equipName);
    commonForm->addRow("Type", equipType);
    commonForm->addRow("Extra text", equipExtraText);

    equipWeaponBox = new QGroupBox("Weapon");
    editorLayout->addWidget(equipWeaponBox);
    QFormLayout* weaponForm = new QFormLayout();
    equipWeaponBox->setLayout(weaponForm);

    equipWeaponProps = new StringListEditor(std::bind(&MainWindow::GetEquipmentWeaponProps, this),
                                              std::bind(&MainWindow::SetEquipmentWeaponProps,
                                                        this,
                                                        std::placeholders::_1));
    weaponForm->addRow("Props", equipWeaponProps);

    equipRange = new QLineEdit();
    weaponForm->addRow("Range", equipRange);

    equipDmgBaseGroup = new QGroupBox("Damage base");
    editorLayout->addWidget(equipDmgBaseGroup);
    QFormLayout* dmgBaseForm = new QFormLayout();
    equipDmgBaseGroup->setLayout(dmgBaseForm);
    equipDmgDice = new QLineEdit();
    equipDmgBonus = new QSpinBox();
    equipDmgBonus->setRange(0, 50);
    equipDmgType = new QLineEdit();
    dmgBaseForm->addRow("Dice (e.g. 1d8)", equipDmgDice);
    dmgBaseForm->addRow("Bonus", equipDmgBonus);
    dmgBaseForm->addRow("Type (e.g. slashing)", equipDmgType);

    equipDmgAltGroup = new QGroupBox("Damage (alternate / versatile)");
    editorLayout->addWidget(equipDmgAltGroup);
    QFormLayout* dmgAltForm = new QFormLayout();
    equipDmgAltGroup->setLayout(dmgAltForm);
    equipAltDmgDice = new QLineEdit();
    equipAltDmgBonus = new QSpinBox();
    equipAltDmgBonus->setRange(0, 50);
    equipAltDmgType = new QLineEdit();
    dmgAltForm->addRow("Dice (e.g. 1d10)", equipAltDmgDice);
    dmgAltForm->addRow("Bonus", equipAltDmgBonus);
    dmgAltForm->addRow("Type (e.g. slashing)", equipAltDmgType);

    editorLayout->addWidget(new QLabel("Extra damages (strings like +1d4 poison)"));
    equipDmgExtra = new StringListEditor(std::bind(&MainWindow::GetEquipmentDamageExtraStrings, this),
                                         std::bind(&MainWindow::SetEquipmentDamageExtraStrings,
                                                   this,
                                                   std::placeholders::_1));
    editorLayout->addWidget(equipDmgExtra);

    equipArmorBox = new QGroupBox("Armor AC");
    editorLayout->addWidget(equipArmorBox);
    QVBoxLayout* armorOuter = new QVBoxLayout();
    equipArmorBox->setLayout(armorOuter);
    QHBoxLayout* acModeRow = new QHBoxLayout();
    acModeRow->addWidget(new QLabel("AC mode:"));
    equipAcModeStat = new QRadioButton("Stat-based AC");
    equipAcModeFix = new QRadioButton("Fixed AC (fixmod)");
    acModeRow->addWidget(equipAcModeStat);
    acModeRow->addWidget(equipAcModeFix);
    acModeRow->addStretch();
    armorOuter->addLayout(acModeRow);

    equipArmorAcStack = new QStackedWidget();
    armorOuter->addWidget(equipArmorAcStack);

    QWidget* statAcPage = new QWidget();
    QFormLayout* statAcForm = new QFormLayout();
    statAcPage->setLayout(statAcForm);
    equipAcBase = new QSpinBox();
    equipAcBase->setRange(1, 50);
    equipAcModStat = new QLineEdit();
    equipAcModCap = new QSpinBox();
    equipAcModCap->setRange(0, 20);
    statAcForm->addRow("Base AC", equipAcBase);
    statAcForm->addRow("Mod stat (e.g. dexterity)", equipAcModStat);
    statAcForm->addRow("Mod cap (0 = none)", equipAcModCap);

    QWidget* fixAcPage = new QWidget();
    QFormLayout* fixAcForm = new QFormLayout();
    fixAcPage->setLayout(fixAcForm);
    equipAcFixMod = new QSpinBox();
    equipAcFixMod->setRange(1, 20);
    fixAcForm->addRow("AC bonus (shield, etc.)", equipAcFixMod);

    equipArmorAcStack->addWidget(statAcPage);
    equipArmorAcStack->addWidget(fixAcPage);

    equipWeaponBox->setVisible(false);
    equipDmgBaseGroup->setVisible(false);
    equipDmgAltGroup->setVisible(false);
    equipDmgExtra->setVisible(false);

    QObject::connect(equipBucket, &QComboBox::currentTextChanged, this, &MainWindow::OnEquipmentBucketOrKindChanged);
    QObject::connect(equipKind, &QComboBox::currentTextChanged, this, &MainWindow::OnEquipmentBucketOrKindChanged);
    QObject::connect(equipItems, &QListWidget::currentRowChanged, this, &MainWindow::OnEquipmentRowChanged);
    QObject::connect(addItem, &QPushButton::clicked, this, &MainWindow::OnEquipmentAddClicked);
    QObject::connect(removeItem, &QPushButton::clicked, this, &MainWindow::OnEquipmentRemoveClicked);

    QObject::connect(equipName, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentCommonFieldChanged);
    QObject::connect(equipType, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentCommonFieldChanged);
    QObject::connect(equipExtraText, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentCommonFieldChanged);

    QObject::connect(equipRange, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentRangeChanged);
    QObject::connect(equipDmgDice, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentDmgDiceChanged);
    QObject::connect(equipDmgBonus, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnEquipmentDmgBonusChanged);
    QObject::connect(equipDmgType, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentDmgTypeChanged);
    QObject::connect(equipAltDmgDice, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentAltDmgDiceChanged);
    QObject::connect(equipAltDmgBonus, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnEquipmentAltDmgBonusChanged);
    QObject::connect(equipAltDmgType, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentAltDmgTypeChanged);

    QObject::connect(equipAcModeStat, &QRadioButton::toggled, this, &MainWindow::OnEquipmentAcModeStatToggled);
    QObject::connect(equipAcModeFix, &QRadioButton::toggled, this, &MainWindow::OnEquipmentAcModeFixToggled);

    QObject::connect(equipAcBase, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnEquipmentAcBaseChanged);
    QObject::connect(equipAcModStat, &QLineEdit::textChanged, this, &MainWindow::OnEquipmentAcModStatChanged);
    QObject::connect(equipAcModCap, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnEquipmentAcModCapChanged);
    QObject::connect(equipAcFixMod, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnEquipmentAcFixModChanged);

    EnsureEquipmentStructure();
    RefreshEquipmentItemList();
    return w;
}

QWidget* MainWindow::BuildBackpackTab()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    w->setLayout(layout);

    EnsureBackpackObject();

    QTabWidget* backpackTabs = new QTabWidget();
    backpackTabs->setStyleSheet(QStringLiteral("QTabWidget::pane { margin: 2px; padding: 4px; }"));
    layout->addWidget(backpackTabs, 1);

    for (size_t i = 0; i < kBackpackSectionKeys.size(); ++i)
    {
        const char* key = kBackpackSectionKeys[i];
        backpackSectionEditors[i] =
            new StringListEditor(std::bind(&MainWindow::GetBackpackSectionItemsForKey, this, key),
                                 std::bind(&MainWindow::SetBackpackSectionItemsForKey,
                                           this,
                                           key,
                                           std::placeholders::_1),
                                 StringListEditor::Mode::SelectRowToEdit);
        backpackTabs->addTab(backpackSectionEditors[i], QString::fromUtf8(kBackpackTabTitles[i]));
    }
    RefreshBackpackSections();

    return w;
}

QWidget* MainWindow::BuildRawJsonTab()
{
    QWidget* w = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    w->setLayout(layout);

    rawJson = new QPlainTextEdit();
    rawJson->setReadOnly(true);
    layout->addWidget(rawJson, 1);

    return w;
}

void MainWindow::RefreshProficienciesFromDocument()
{
    if (!profBonusSpin) { return; }

    EnsureSavingThrowsInitialized();
    EnsureSkillsInitialized();

    const nlohmann::ordered_json& root = std::as_const(doc).Json();
    const auto& prof =
        root.contains("proficiencies") && root["proficiencies"].is_object() ? root["proficiencies"]
                                                                            : nlohmann::ordered_json::object();

    profBonusSpin->blockSignals(true);
    profBonusSpin->setValue(GetIntOrDefault(prof, "bonus", 2));
    profBonusSpin->blockSignals(false);

    static const char* statOrder[] = {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"};
    const auto& saving =
        prof.contains("savingThrows") && prof["savingThrows"].is_object() ? prof["savingThrows"] : nlohmann::ordered_json::object();
    for (size_t i = 0; i < profSavingThrowSpins.size(); ++i)
    {
        QSpinBox* s = profSavingThrowSpins[i];
        if (!s) { continue; }
        s->blockSignals(true);
        s->setValue(GetIntOrDefault(saving, statOrder[i], 0));
        s->blockSignals(false);
    }

    const auto& skillsRoot =
        prof.contains("skills") && prof["skills"].is_object() ? prof["skills"] : nlohmann::ordered_json::object();
    for (QSpinBox* s : profSkillSpins)
    {
        if (!s) { continue; }
        const QByteArray stat = s->property("statKey").toByteArray();
        const QByteArray skill = s->property("skillKey").toByteArray();
        if (stat.isEmpty() || skill.isEmpty()) { continue; }
        int v = 0;
        if (skillsRoot.contains(stat.constData()) && skillsRoot[stat.constData()].is_object())
        {
            v = GetIntOrDefault(skillsRoot[stat.constData()], skill.constData(), 0);
        }
        s->blockSignals(true);
        s->setValue(v);
        s->blockSignals(false);
    }

    if (profLanguagesEditor) { profLanguagesEditor->Refresh(); }
    if (profToolsEditor) { profToolsEditor->Refresh(); }
    if (profArmorsEditor) { profArmorsEditor->Refresh(); }
    if (profSimpleWeaponsEditor) { profSimpleWeaponsEditor->Refresh(); }
    if (profMartialWeaponsEditor) { profMartialWeaponsEditor->Refresh(); }
}

void MainWindow::UpdateTabsFromDocument()
{
    const nlohmann::ordered_json& root = std::as_const(doc).Json();
    if (generalName) { generalName->setText(QString::fromStdString(GetStringOrDefault(root, "name", "New Character"))); }
    if (generalNameSpacing)
    {
        generalNameSpacing->blockSignals(true);
        generalNameSpacing->setValue(GetDoubleOrDefault(root, "nameLetterSpacing", 1.0));
        generalNameSpacing->blockSignals(false);
    }
    if (generalRace) { generalRace->setText(QString::fromStdString(GetStringOrDefault(root, "race", "Human"))); }
    if (generalBackground)
    {
        generalBackground->setText(QString::fromStdString(GetStringOrDefault(root, "background", "")));
    }

    const auto& stats = root.contains("stats") ? root.at("stats") : nlohmann::ordered_json();
    SetSpinValue(statStrength, stats, "strength", 0);
    SetSpinValue(statDexterity, stats, "dexterity", 0);
    SetSpinValue(statConstitution, stats, "constitution", 0);
    SetSpinValue(statIntelligence, stats, "intelligence", 0);
    SetSpinValue(statWisdom, stats, "wisdom", 0);
    SetSpinValue(statCharisma, stats, "charisma", 0);
    SetSpinValue(statSpeed, stats, "speed", 30);
    SetSpinValue(statMaxHp, stats, "maxHp", 10);
    SetSpinValue(statAc, stats, "ac", 14);
    SetSpinValue(statInitiativeBonus, stats, "initiativeBonus", 0);
    SetSpinValue(statPPercBonus, stats, "pPercBonus", 0);

    if (generalTraitsEditor) { generalTraitsEditor->Refresh(); }

    RefreshProficienciesFromDocument();

    if (classesList)
    {
        RefreshClassList();
        if (!classesList->currentItem() && classesList->count() > 0) { classesList->setCurrentRow(0); }
        LoadSelectedClassIntoEditor();
    }

    if (equipItems)
    {
        RefreshEquipmentItemList();
        if (equipItems->currentRow() < 0 && equipItems->count() > 0) { equipItems->setCurrentRow(0); }
        LoadSelectedEquipmentIntoEditor();
    }

    if (backpackSectionEditors[0]) { RefreshBackpackSections(); }
}

void MainWindow::RefreshFileList()
{
    const bool hadPath = doc.FilePath().has_value();
    const std::string savedPath = hadPath ? doc.FilePath().value() : std::string();

    QSignalBlocker block(fileList);
    fileList->clear();
    const auto files = repo.ListCharacterFiles();
    for (const auto& f : files)
    {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(f.filename));
        item->setData(Qt::UserRole, QString::fromStdString(f.fullPath));
        fileList->addItem(item);
    }
    fileList->setCurrentRow(-1);

    if (editorWorkspaceOpen_ && hadPath)
    {
        const QString want = QString::fromStdString(savedPath);
        for (int i = 0; i < fileList->count(); ++i)
        {
            QListWidgetItem* it = fileList->item(i);
            if (it && it->data(Qt::UserRole).toString() == want)
            {
                fileList->setCurrentRow(i);
                break;
            }
        }
    }
}

bool MainWindow::TrySaveCurrentDocument()
{
    if (!doc.IsDirty()) { return true; }
    Save();
    return !doc.IsDirty();
}

bool MainWindow::MaybeDiscardUnsavedChanges()
{
    if (!doc.IsDirty()) { return true; }
    const int r = QMessageBox::warning(this,
                                       "Unsaved changes",
                                       "The current character has unsaved changes. What would you like to do?",
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                       QMessageBox::Save);
    if (r == QMessageBox::Cancel) { return false; }
    if (r == QMessageBox::Discard) { return true; }
    return TrySaveCurrentDocument();
}

void MainWindow::OnCharacterFileListCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (suppressFileListNavigation) { return; }

    if (fileListLoadsNeedUserGesture_)
    {
        if (current != nullptr)
        {
            suppressFileListNavigation = true;
            fileList->clearSelection();
            fileList->setCurrentRow(-1);
            suppressFileListNavigation = false;
        }
        return;
    }

    if (!current) { return; }
    const QString fullPath = current->data(Qt::UserRole).toString();
    if (fullPath.isEmpty()) { return; }

    if (doc.FilePath().has_value() && fullPath.toStdString() == doc.FilePath().value() && !doc.IsDirty()) { return; }

    if (!MaybeDiscardUnsavedChanges())
    {
        suppressFileListNavigation = true;
        if (previous) { fileList->setCurrentItem(previous); }
        else { fileList->clearSelection(); }
        suppressFileListNavigation = false;
        return;
    }

    LoadJsonFromPath(fullPath.toStdString());
}

void MainWindow::LoadJsonFromPath(const std::string& fullPath)
{
    try
    {
        nlohmann::ordered_json j = repo.Load(fullPath);
        SetDocument(CharacterDocument(std::move(j)), fullPath);
        SetEditorWorkspaceOpen(true);
        for (const auto& issue : CharacterSchema::Validate(std::as_const(doc).Json()))
        {
            if (issue.severity == ValidationIssue::Severity::Error)
            {
                QMessageBox::warning(
                    this,
                    "Validation",
                    QString::fromStdString(issue.jsonPath + ": " + issue.message));
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Load failed", e.what());
        suppressFileListNavigation = true;
        if (doc.FilePath().has_value())
        {
            const QString want = QString::fromStdString(doc.FilePath().value());
            for (int i = 0; i < fileList->count(); ++i)
            {
                QListWidgetItem* it = fileList->item(i);
                if (it && it->data(Qt::UserRole).toString() == want)
                {
                    fileList->setCurrentRow(i);
                    suppressFileListNavigation = false;
                    return;
                }
            }
        }
        fileList->clearSelection();
        suppressFileListNavigation = false;
    }
}

void MainWindow::NewCharacter()
{
    SetDocument(CharacterDocument(CharacterSchema::MakeDefaultCharacter()), std::nullopt);
    SetEditorWorkspaceOpen(true);
    suppressFileListNavigation = true;
    fileList->clearSelection();
    suppressFileListNavigation = false;
}

void MainWindow::Save()
{
    if (!editorWorkspaceOpen_)
    {
        QMessageBox::information(this, "Character editor", "Select a character file or choose New first.");
        return;
    }
    if (!doc.FilePath().has_value())
    {
        SaveAs();
        return;
    }

    CharacterSchema::NormalizeInPlace(doc.Json());

    const auto issues = CharacterSchema::Validate(doc.Json());
    for (const auto& issue : issues)
    {
        if (issue.severity == ValidationIssue::Severity::Error)
        {
            QMessageBox::critical(this, "Validation failed", QString::fromStdString(issue.jsonPath + ": " + issue.message));
            return;
        }
    }

    try
    {
        repo.SaveAtomic(doc.FilePath().value(), doc.Json());
        doc.MarkClean();
        UpdateValidationSummary();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Save failed", e.what());
    }
}

void MainWindow::SaveAs()
{
    if (!editorWorkspaceOpen_)
    {
        QMessageBox::information(this, "Character editor", "Select a character file or choose New first.");
        return;
    }
    const QString filename = QInputDialog::getText(
        this, "Save As", "Filename (will be saved under assets/cfg):", QLineEdit::Normal, "char_New.json");
    if (filename.isEmpty()) { return; }

    std::string name = filename.toStdString();
    if (name.rfind("char_", 0) != 0) { name = "char_" + name; }
    if (name.size() < 5 || name.substr(name.size() - 5) != ".json") { name += ".json"; }

    const std::string fullPath = repo.RootDir() + "/" + name;

    CharacterSchema::NormalizeInPlace(doc.Json());

    const auto issues = CharacterSchema::Validate(doc.Json());
    for (const auto& issue : issues)
    {
        if (issue.severity == ValidationIssue::Severity::Error)
        {
            QMessageBox::critical(this, "Validation failed", QString::fromStdString(issue.jsonPath + ": " + issue.message));
            return;
        }
    }

    try
    {
        repo.SaveAtomic(fullPath, doc.Json());
        doc.SetFilePath(fullPath);
        doc.MarkClean();
        RefreshFileList();
        UpdateValidationSummary();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Save failed", e.what());
    }
}

void MainWindow::SetDocument(CharacterDocument docParam, std::optional<std::string> filePath)
{
    doc = std::move(docParam);
    doc.SetFilePath(std::move(filePath));
    CharacterSchema::NormalizeInPlace(doc.Json());
    UpdateTabsFromDocument();
    UpdateRawJsonView();
    UpdateValidationSummary();
    doc.MarkClean();
}

void MainWindow::UpdateRawJsonView()
{
    if (!rawJson) { return; }
    rawJson->setPlainText(QString::fromStdString(std::as_const(doc).Json().dump(4)));
}

void MainWindow::SetEditorWorkspaceOpen(bool open)
{
    editorWorkspaceOpen_ = open;
    if (editorStack) { editorStack->setCurrentIndex(open ? 1 : 0); }
    if (!open) { fileListLoadsNeedUserGesture_ = true; }
    UpdateValidationSummary();
}

void MainWindow::UpdateValidationSummary()
{
    if (!validationLabel) { return; }
    if (!editorWorkspaceOpen_)
    {
        validationLabel->setText(
            "No character loaded.\n"
            "Select a file from the list, or choose New to create one.");
        return;
    }

    const auto issues = CharacterSchema::Validate(std::as_const(doc).Json());
    int errors = 0;
    int warnings = 0;
    for (const auto& i : issues)
    {
        if (i.severity == ValidationIssue::Severity::Error) { errors++; }
        else { warnings++; }
    }

    std::string text = "Document: ";
    text += doc.FilePath().has_value() ? doc.FilePath().value() : "(new / unsaved)";
    text += "\nValidation: " + std::to_string(errors) + " errors, " + std::to_string(warnings) + " warnings";

    validationLabel->setText(QString::fromStdString(text));
}

}

