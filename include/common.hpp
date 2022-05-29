#ifndef BHBB_COMMON_H
#define BHBB_COMMON_H

#include "parser.hpp"
#include "pages/home_page.hpp"

#define PLUGIN_NAME "bhbb_plugin"
#define RESOURCE_PATH "app0:resource/bhbb_plugin.rco"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"
#define CBPSDB_URL "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv"
#define CBPSDB_DOWNLOAD_ICONS_URL "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/blob/main/icons.zip?raw=true"
#define VITADB_URL "https://rinnegatamante.it/vitadb/list_hbs_json.php"
#define VITADB_DOWNLOAD_ICONS_URL "https://vitadb.rinnegatamante.it/icons_zip.php"
#define VITADB_ICON_URL "https://rinnegatamante.it/vitadb/icons/"
#define VITADB_SCREENSHOTS_URL "https://rinnegatamante.it/vitadb"
#define DATA_PATH "ux0:data/betterHomebrewBrowser"
#define SCREENSHOT_SAVE_PATH DATA_PATH "/screenshots"

#define TEMP_PATH "ux0:/temp"

#define ICON_SAVE_PATH "ux0:data/betterHomebrewBrowser/icons"
#define CBPSDB_ICON_ZIP_SAVE_PATH TEMP_PATH "/bhbb_cbps_db_icons.zip"
#define CBPSDB_ICON_SAVE_PATH ICON_SAVE_PATH "/cbpsdb"
#define VITADB_ICON_ZIP_SAVE_PATH TEMP_PATH "/bhbb_vita_db_icons.zip"
#define VITADB_ICON_SAVE_PATH ICON_SAVE_PATH "/vitadb"

#define CBPSDB_TIME_SAVE_PATH "ux0:data/betterHomebrewBrowser/cbpsdb_icon_download"
#define VITADB_TIME_SAVE_PATH "ux0:data/betterHomebrewBrowser/vitadb_icon_download"
#define CONFIG_SAVE_PATH "ux0:data/betterHomebrewBrowser/config"

#define POPUP_DIALOG_BG "popup_diag_bg"
#define POPUP_DIALOG_ID "popup_dialog"
#define POPUP_DIALOG_BUTTON "diag_button_template"
#define POPUP_DIALOG_BOX "diag_box_template"

#define BLANK_PAGE_ID "blank_page_template"

#define ICON_MISSING_TEX_ID "tex_missing_icon"

#define APPS_PER_LOAD 100

#define MUSIC_PATH "pd0:data/systembgm/store.at9"

extern parser::HomebrewList list;

extern paf::Plugin *mainPlugin;

extern paf::graphics::Surface *BrokenTex;
extern paf::graphics::Surface *TransparentTex;

extern int loadFlags;

extern paf::ui::CornerButton *g_backButton;
extern paf::ui::CornerButton *g_forwardButton;
extern paf::ui::BusyIndicator *g_busyIndicator;

static paf::ui::Widget *s_currentPage;

extern home::Page *g_homePage;

#endif