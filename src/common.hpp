#ifndef BHBB_COMMON_H
#define BHBB_COMMON_H

#include <paf.h>
#include "pagemgr.hpp"
#include "parser.hpp"
#include "configmgr.hpp"

#define SCENE_NAME "page_main"
#define PLUGIN_NAME "bhbb_plugin"
#define RESOURCE_PATH "app0:resource/bhbb_plugin.rco"

#define BUSY_INICATOR_ID "busy_indicator"
#define BACK_BUTTON_ID "back_button"
#define FORWARD_BUTTON_ID "forward_button"
#define SETTINGS_BUTTON_ID "settings_button"
#define MAIN_PLANE_ID "plane_common"
#define MAIN_PAGE_ID "page_common"
#define PAGE_TITLE_ID "main_titlebar_text"
#define PAGE_ROOT_ID "page_root_plane"

#define LIST_TEMPLATE_ID "list_template"
#define LIST_TEMPLATE_ID_NO_TITLE "list_template_no_title"
#define LIST_ROOT_PLANE "list_plane_bg"
#define LIST_SCROLL_VIEW "list_scroll_view"
#define LIST_SCROLL_BOX "list_scroll_box"
#define LIST_BUTTON_TEMPLATE "list_button_template_no_img"
#define LIST_BUTTON_LONG_TEMPLATE "list_button_long_template_no_img"

#define LIST_BUTTON_TEMPLATE_IMG "list_button_template"
#define LIST_BUTTON_LONG_TEMPLATE_IMG "list_button_long_template"

#define SELECTION_LIST_TEMPLATE "selection_template"
#define SELECTION_LIST_NO_TITLE_TEMPLATE "selection_template_no_title"
#define SELECTION_LIST_TOP_BUTTON_TEMPLATE "selection_list_top_button_template"
#define SELECTION_LIST_TOP_BUTTON_PLANE_ID "buttons_plane"

#define LOADING_PAGE_ID "loading_template"
#define LOADING_PAGE_TEXT "info_text"

#define INFO_PAGE_ID "info_page"
#define INFO_PAGE_ICON_ID "hb_icon"
#define INFO_PAGE_DESCRIPTION_TEXT_ID "hb_description"
#define INFO_PAGE_CREDITS_TEXT_ID "hb_credits"
#define INFO_PAGE_SCREENSHOT_ID "hb_screenshot"
#define INFO_PAGE_DOWNLOAD_BUTTON_ID "download_button"
#define INFO_PAGE_VERSION_TEXT_ID "hb_version"
#define INFO_PAGE_SIZE_TEXT_ID "hb_size"

#define TEXT_PAGE_NO_TITLE_ID "text_page_no_title"
#define TEXT_PAGE_ID "text_page"
#define TEXT_PAGE_TEXT "info_text"
#define TEXT_PAGE_TITLE_TEXT "main_titlebar_text"

#define PROGRESS_PAGE_ID "progress_template"
#define PROGRESS_PAGE_BAR "loading_bar"
#define PROGRESS_BAR_PAGE_PLANE "progressbar_plane"
#define PROGRESS_PAGE_BAR_TEMPLATE "progress_page_bar_template"

#define PICTURE_PAGE_ID "picture_list_page_template"
#define PICTURE_PAGE_PICTURE_TEMPLATE "picture_page_picture_template"

#define DECISION_PAGE_ID "decision_page_template"
#define DECISION_PAGE_CONFIRM_BUTTON_ID "button_confirm"
#define DECISION_PAGE_DECLINE_BUTTON_ID "button_decline"
#define DECISION_PAGE_TEXT_ID "info_text"

#define HOMEBREW_LIST_PAGE_ID "homebrew_list_page_template"
#define HOMEBREW_LIST_PAGE_LIST_PLANE_ID "list_plane"
#define HOMEBREW_LIST_TEMPLATE_ID "homebrew_list_template"
#define HOMEBREW_LIST_PAGE_UTIL_BUTTON_ID "util_button"
#define HOMEBREW_LIST_PAGE_EMU_BUTTON_ID "emu_button"
#define HOMEBREW_LIST_PAGE_ALL_BUTTON_ID "all_button"
#define HOMEBREW_LIST_PAGE_PORT_BUTTON_ID "port_button"
#define HOMEBREW_LIST_PAGE_GAME_BUTTON_ID "game_button"
#define HOMEBREW_LIST_PAGE_SEARCH_BUTTON_ID "search_button"

#define SEARCH_PAGE_TEMPLATE_ID "search_page_template"
#define SEARCH_PAGE_SEARCH_BOX_ID "search_box"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"
#define CBPSDB_URL "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv"
#define CBPSDB_DOWNLOAD_ICONS_URL "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/blob/main/icons.zip?raw=true"
#define VITADB_URL "https://rinnegatamante.it/vitadb/list_hbs_json.php"
#define VITADB_DOWNLOAD_ICONS_URL "https://vitadb.rinnegatamante.it/icons_zip.php"
#define VITADB_ICON_URL "https://rinnegatamante.it/vitadb/icons/"
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

#define APPS_PER_PAGE 50

#define userDbDefault CBPSDB

extern CornerButton *mainBackButton;
extern CornerButton *settingsButton;
extern CornerButton *forwardButton;
extern Plugin *mainPlugin;
extern Plane *mainRoot;
extern LinkedList list;
extern graphics::Texture *BrokenTex;
extern graphics::Texture *transparentTex;
extern Widget *mainScene;
extern userConfig conf;
extern int loadFlags;

extern BackButtonEventHandler *mainBackButtonEvent;
extern ForwardButtonEventHandler *mainForwardButtonEvent;

extern Allocator *fwAllocator;

extern int pageNum;
extern int category;

#endif