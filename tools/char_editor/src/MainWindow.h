#pragma once

#include <QMainWindow>

#include "CharacterDocument.h"
#include "CharacterRepository.h"

#include <array>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

class QTreeWidget;
class QListWidget;
class QPlainTextEdit;
class QLabel;
class QTabWidget;
class QWidget;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QString;
class QVBoxLayout;
class QComboBox;
class QGroupBox;
class QListWidgetItem;
class QTreeWidgetItem;
class QStackedWidget;
class QRadioButton;
class QCheckBox;
class QEvent;

namespace CharEditor
{

class StringListEditor;

class MainWindow : public QMainWindow
{
  public:
    explicit MainWindow(CharacterRepository repo);
    ~MainWindow() override;

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    // UI event handlers (no lambdas)
    void OnToolbarNew();
    void OnToolbarSave();
    void OnToolbarSaveAs();
    void OnToolbarGenerateAll();
    void OnToolbarGenerateCurrentFolder();
    void OnToolbarViewOnWeb();
    void OnRefreshFileListButton();

    void OnRootStringChanged(const QString& value);
    void OnRootIntChanged(int value);
    void OnRootDoubleChanged(double value);
    void OnStatIntChanged(int value);
    void OnProficiencyBonusChanged(int value);
    void OnSavingThrowChanged(int value);
    void OnSkillValueChanged(int value);

    // Helpers (no lambdas)
    std::vector<std::string> GetProficiencyArray(const std::string& key);
    void SetProficiencyArray(const std::string& key, const std::vector<std::string>& items);
    void EnsureSavingThrowsInitialized();
    void EnsureSkillsInitialized();
    QSpinBox* CreateSkillSpinBox(const char* statKey, const char* skillKey);
    QGroupBox* BuildSkillStatGroup(const char* title, const char* statKey, const std::vector<const char*>& skillKeys);

    void SetSpinValue(QSpinBox* spin, const nlohmann::ordered_json& obj, const char* key, int def);

    std::vector<std::string> GetSelectedClassSpells();
    void SetSelectedClassSpells(const std::vector<std::string>& items);

    void RefreshClassList();
    void LoadSelectedClassIntoEditor();
    void OnClassesAddClicked();
    void OnClassesRemoveClicked();
    void OnClassListCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void OnClassIdEditingFinished();
    void OnClassSpellLevelComboChanged(const QString& text);
    void OnClassLevelSpinChanged(int value);
    void OnClassResourcePointsChanged(int value);
    void OnClassHitDiceChanged(const QString& value);
    void OnClassSubclassChanged(const QString& value);
    void OnClassCastStatChanged(const QString& value);
    void OnClassSpellSlotChanged(int value);

    void EnsureEquipmentStructure();
    nlohmann::ordered_json* CurrentEquipmentArray();
    nlohmann::ordered_json* CurrentEquipmentItem();
    void RefreshEquipmentItemList();
    void LoadSelectedEquipmentIntoEditor();
    void OnEquipmentBucketOrKindChanged(const QString& text);
    void OnEquipmentRowChanged(int row);
    void OnEquipmentAddClicked();
    void OnEquipmentRemoveClicked();
    void OnEquipmentCommonFieldChanged(const QString& value);
    void OnEquipmentAcBaseChanged(int value);
    void OnEquipmentAcModStatChanged(const QString& value);
    void OnEquipmentAcModCapChanged(int value);
    void OnEquipmentAcCapEnabledChanged(bool checked);
    void OnEquipmentAcFixModChanged(int value);
    void OnEquipmentAcModeStatToggled(bool checked);
    void OnEquipmentAcModeFixToggled(bool checked);
    void ApplyArmorAcModeToDocument(bool statBased);
    void WriteStatBasedArmorAcToDocument();
    void OnEquipmentRangeChanged(const QString& value);
    void OnEquipmentDmgDiceChanged(const QString& value);
    void OnEquipmentDmgBonusChanged(int value);
    void OnEquipmentDmgTypeChanged(const QString& value);
    void OnEquipmentAltDmgDiceChanged(const QString& value);
    void OnEquipmentAltDmgBonusChanged(int value);
    void OnEquipmentAltDmgTypeChanged(const QString& value);
    std::vector<std::string> GetEquipmentWeaponProps();
    void SetEquipmentWeaponProps(const std::vector<std::string>& items);
    std::vector<std::string> GetEquipmentDamageExtraStrings();
    void SetEquipmentDamageExtraStrings(const std::vector<std::string>& items);

    void EnsureBackpackObject();
    std::vector<std::string> GetBackpackSectionItemsForKey(const char* sectionKey);
    void SetBackpackSectionItemsForKey(const char* sectionKey, const std::vector<std::string>& items);
    void RefreshBackpackSections();

    std::vector<std::string> GetTraits();
    void SetTraits(const std::vector<std::string>& items);

    void RefreshFileList();
    void OnCharacterFileTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void SelectFileTreeItemByPath(const QString& fullPath);
    bool MaybeDiscardUnsavedChanges();
    bool TrySaveCurrentDocument();
    void LoadJsonFromPath(const std::string& fullPath);
    void NewCharacter();
    void Save();
    void SaveAs();

    void SetDocument(CharacterDocument doc, std::optional<std::string> filePath);
    void BuildTabs();
    QWidget* BuildGeneralTab();
    QWidget* BuildProficienciesTab();
    QWidget* BuildClassesTab();
    QWidget* BuildEquipmentTab();
    QWidget* BuildBackpackTab();
    QWidget* BuildRawJsonTab();

    void UpdateTabsFromDocument();
    void RefreshProficienciesFromDocument();
    void UpdateRawJsonView();
    void UpdateValidationSummary();
    void SetEditorWorkspaceOpen(bool open);

    QString ProjectRootAbsolute() const;
    QString ResolvePdfAppPath(const QString& projectRoot) const;
    QString ResolveWebrenderBaseUrl() const;
    QString DeriveCampaignFromPath(const QString& filePath) const;
    QString DeriveWebrenderSlugFromPath(const QString& filePath) const;
    void RunPdfGeneratorAndShowResult(const QString& projectRoot, const QStringList& args);
    void OpenCharacterOnWeb();

    CharacterRepository repo;
    CharacterDocument doc;
    bool suppressFileListNavigation = false;
    /// Ignore file-list selection until the user clicks or tabs into the list (avoids auto-load on focus).
    bool fileListLoadsNeedUserGesture_ = true;
    bool editorWorkspaceOpen_ = false;

    QTreeWidget* fileList = nullptr;
    QStackedWidget* editorStack = nullptr;
    QTabWidget* tabs = nullptr;

    // Raw JSON tab
    QPlainTextEdit* rawJson = nullptr;
    QLabel* validationLabel = nullptr;

    // General tab (includes stats column; for refresh)
    QWidget* generalTab = nullptr;
    QLineEdit* generalName = nullptr;
    QDoubleSpinBox* generalNameSpacing = nullptr;
    QLineEdit* generalRace = nullptr;
    QLineEdit* generalBackground = nullptr;
    StringListEditor* generalTraitsEditor = nullptr;

    // Stats widgets (General tab, right column)
    QSpinBox* statStrength = nullptr;
    QSpinBox* statDexterity = nullptr;
    QSpinBox* statConstitution = nullptr;
    QSpinBox* statIntelligence = nullptr;
    QSpinBox* statWisdom = nullptr;
    QSpinBox* statCharisma = nullptr;
    QSpinBox* statSpeed = nullptr;
    QSpinBox* statMaxHp = nullptr;
    QSpinBox* statAcBonus = nullptr;
    QSpinBox* statInitiativeBonus = nullptr;
    QSpinBox* statPPercBonus = nullptr;

    // Proficiencies tab widgets (for refresh)
    QWidget* profTab = nullptr;
    QSpinBox* profBonusSpin = nullptr;
    StringListEditor* profLanguagesEditor = nullptr;
    StringListEditor* profToolsEditor = nullptr;
    StringListEditor* profArmorsEditor = nullptr;
    StringListEditor* profSimpleWeaponsEditor = nullptr;
    StringListEditor* profMartialWeaponsEditor = nullptr;
    std::array<QSpinBox*, 6> profSavingThrowSpins{};
    std::vector<QSpinBox*> profSkillSpins;

    // Classes tab
    QListWidget* classesList = nullptr;
    QLineEdit* classIdEdit = nullptr;
    QComboBox* classesSpellLevel = nullptr;
    StringListEditor* classesSpellsEditor = nullptr;
    QSpinBox* classLevelSpin = nullptr;
    QLineEdit* classHitDiceEdit = nullptr;
    QLineEdit* classSubclassEdit = nullptr;
    QLineEdit* classCastStatEdit = nullptr;
    QSpinBox* classResourceSpin = nullptr;
    std::array<QSpinBox*, 9> classSlotSpins{};

    // Equipment tab
    QComboBox* equipBucket = nullptr;
    QComboBox* equipKind = nullptr;
    QListWidget* equipItems = nullptr;
    QLineEdit* equipName = nullptr;
    QLineEdit* equipType = nullptr;
    QLineEdit* equipExtraText = nullptr;
    QGroupBox* equipWeaponBox = nullptr;
    StringListEditor* equipWeaponProps = nullptr;
    QLineEdit* equipRange = nullptr;
    QGroupBox* equipDmgBaseGroup = nullptr;
    QLineEdit* equipDmgDice = nullptr;
    QSpinBox* equipDmgBonus = nullptr;
    QLineEdit* equipDmgType = nullptr;
    QGroupBox* equipDmgAltGroup = nullptr;
    QLineEdit* equipAltDmgDice = nullptr;
    QSpinBox* equipAltDmgBonus = nullptr;
    QLineEdit* equipAltDmgType = nullptr;
    StringListEditor* equipDmgExtra = nullptr;
    QGroupBox* equipArmorBox = nullptr;
    QRadioButton* equipAcModeStat = nullptr;
    QRadioButton* equipAcModeFix = nullptr;
    QStackedWidget* equipArmorAcStack = nullptr;
    QSpinBox* equipAcBase = nullptr;
    QLineEdit* equipAcModStat = nullptr;
    QSpinBox* equipAcModCap = nullptr;
    QCheckBox* equipAcCapEnabled = nullptr;
    QSpinBox* equipAcFixMod = nullptr;

    // Backpack tab (fixed sections: accessories, consumables, kits & tools, general)
    std::array<StringListEditor*, 4> backpackSectionEditors{};
};

}

