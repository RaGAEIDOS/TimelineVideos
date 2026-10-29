"""
i18n.py
-------
Arabic / English translations + RTL support.

The whole UI reads its text through tr("some.key"). Switching
language at runtime is a one-liner:

    set_language("ar")
    app.setLayoutDirection(layout_direction())

…then every widget calls its retranslate() method to re-read its
texts. The choice is persisted by the caller (QSettings).

Numbers/dates stay Western-digits (1, 2, 3…) in both languages on
purpose — mixing digit systems breaks sorting-by-eye and the
duration/size formatting is shared with the stats bar.
"""

from typing import Dict

LANG_AR = "ar"
LANG_EN = "en"
_DEFAULT_LANG = LANG_AR  # user's choice: Arabic RTL out of the box

_current_lang = _DEFAULT_LANG

# Each entry: key -> {lang: text}. Use .format(**vars) on the result.
_STRINGS: Dict[str, Dict[str, str]] = {
    # ---------- menu bar ----------
    "menu.file":            {"ar": "ملف", "en": "File"},
    "menu.open_folder":     {"ar": "فتح مجلد…", "en": "Open Folder…"},
    "menu.youtube_import":  {"ar": "استيراد بلاي ليست يوتيوب…", "en": "Import YouTube Playlist…"},
    "menu.language":        {"ar": "اللغة", "en": "Language"},
    "menu.lang_ar":         {"ar": "العربية", "en": "عربي"},
    "menu.lang_en":         {"ar": "English", "en": "English"},
    "menu.exit":            {"ar": "خروج", "en": "Exit"},
    "menu.help":            {"ar": "مساعدة", "en": "Help"},
    "menu.about":           {"ar": "عن البرنامج", "en": "About"},

    # ---------- header ----------
    "btn.language":         {"ar": "English", "en": "عربي"},
    "btn.language_tip":     {"ar": "تغيير اللغة", "en": "Switch language"},

    # ---------- folder line ----------
    "folder.none":          {"ar": "لا يوجد مجلد مفتوح — تُستخدم القائمة الافتراضية",
                             "en": "No folder open — using the default list"},
    "folder.label":         {"ar": "المجلد: {path}", "en": "Folder: {path}"},

    # ---------- action buttons ----------
    "btn.add":              {"ar": "＋ إضافة", "en": "＋ Add"},
    "btn.remove":           {"ar": "－ حذف", "en": "－ Remove"},
    "btn.open_folder":      {"ar": "فتح مجلد", "en": "Open Folder"},
    "btn.play":             {"ar": "▶ تشغيل المحدد", "en": "▶ Play Selected"},
    "btn.youtube":          {"ar": "🔗 ترتيب يوتيوب", "en": "🔗 YouTube Order"},
    "btn.youtube_tip":      {"ar": "رتّب القائمة زي ترتيب بلاي ليست يوتيوب",
                             "en": "Reorder the list to match a YouTube playlist"},

    # ---------- sort combo ----------
    "sort.label":           {"ar": "الترتيب:", "en": "Sort:"},
    "sort.tip":             {"ar": "اختر طريقة ترتيب الفيديوهات",
                             "en": "Choose how videos are ordered"},
    "sort.manual":          {"ar": "يدوي (اسحب)", "en": "Manual (drag)"},
    "sort.hash_number":     {"ar": "رقم بعد #", "en": "Number after #"},
    "sort.name":            {"ar": "الاسم", "en": "Name"},
    "sort.number":          {"ar": "رقم الحلقة", "en": "Episode number"},
    "sort.size":            {"ar": "الحجم", "en": "Size"},
    "sort.added_at":        {"ar": "تاريخ الإضافة", "en": "Date added"},
    "sort.last_opened":     {"ar": "آخر تشغيل", "en": "Last played"},

    # ---------- stats bar ----------
    "stats.total":          {"ar": "إجمالي الفيديوهات", "en": "Total videos"},
    "stats.watched":        {"ar": "تمت مشاهدته", "en": "Watched"},
    "stats.queued":         {"ar": "في قائمة الانتظار", "en": "Queued"},
    "stats.total_time":     {"ar": "المدة الكلية", "en": "Total time"},
    "stats.remaining":      {"ar": "المتبقي للمشاهدة", "en": "Time remaining"},
    "stats.videos":         {"ar": "{n} فيديو", "en": "{n} videos"},
    "stats.unknown_suffix": {"ar": "(+{n} بلا مدة)", "en": "(+{n} unprobed)"},
    "stats.progress_tip":   {"ar": "نسبة المشاهدة من إجمالي عدد الفيديوهات",
                             "en": "Watched share of all videos"},

    # ---------- hint ----------
    "hint.text":            {"ar": "دبل كليك لتشغيل الفيديو في مشغّلك الافتراضي. "
                                   "شيك المشاهدة بيسجّلها في الداتابيز (زي اليوتيوب) من غير ما ينقل الملف. "
                                   "أسماء ملفات الترجمة (SRT/VTT…) بتظهر جنب الفيديو بالأخضر.",
                             "en": "Double-click plays a video in your default player. "
                                   "The watched checkbox records it in the database "
                                   "(YouTube-style) without moving the file. "
                                   "Subtitle files (SRT/VTT…) are listed in green "
                                   "next to each video."},

    # ---------- playlist context menu ----------
    "ctx.play":             {"ar": "تشغيل", "en": "Play"},
    "ctx.mark_watched":     {"ar": "تعليم كـ «تمت المشاهدة»", "en": "Mark as watched"},
    "ctx.mark_unwatched":   {"ar": "تعليم كـ «لم تُشاهد»", "en": "Mark as unwatched"},
    "ctx.remove":           {"ar": "حذف من القائمة", "en": "Remove from list"},
    "ctx.open_folder":      {"ar": "فتح مجلد الملف", "en": "Open file's folder"},

    # ---------- messages ----------
    "msg.no_selection.t":   {"ar": "لا يوجد تحديد", "en": "No selection"},
    "msg.no_selection.x":   {"ar": "اختر فيديو من القائمة الأول.", "en": "Pick a video from the list first."},
    "msg.file_missing.t":   {"ar": "الملف غير موجود", "en": "File not found"},
    "msg.file_missing.x":   {"ar": "مقدرتش ألاقي:\n{path}", "en": "Couldn't find:\n{path}"},
    "msg.cant_open.t":      {"ar": "تعذر فتح الملف", "en": "Couldn't open file"},
    "msg.cant_update.t":    {"ar": "تعذر التحديث", "en": "Couldn't update"},
    "msg.cant_update.x":    {"ar": "مقدرتش أحدّث حالة المشاهدة.",
                             "en": "Couldn't update the watched state."},

    # ---------- about ----------
    "about.title":          {"ar": "عن {app}", "en": "About {app}"},

    # ---------- splash screen ----------
    "splash.tagline":       {"ar": "يُجهّز كل شيء قبل البدء…", "en": "Preparing everything before launch…"},
    "splash.step.theme":    {"ar": "تحميل الثيم", "en": "Loading the theme"},
    "splash.step.icons":    {"ar": "تحميل الأيقونات", "en": "Loading the icons"},
    "splash.step.ui":       {"ar": "تحميل وحدات الواجهة", "en": "Loading UI modules"},
    "splash.step.database": {"ar": "تهيئة قاعدة البيانات", "en": "Opening the database"},
    "splash.step.settings": {"ar": "تحميل الإعدادات", "en": "Loading settings"},
    "splash.step.scan":     {"ar": "فحص ملفات الفيديو", "en": "Scanning video files"},
    "splash.step.probe":    {"ar": "فحص أدوات الوسائط (ffprobe / yt-dlp)", "en": "Checking media tools (ffprobe / yt-dlp)"},
    "splash.step.ready":    {"ar": "جاهز!", "en": "Ready!"},

    # ---------- youtube import dialog ----------
    "yt.title":             {"ar": "استيراد بلاي ليست يوتيوب", "en": "Import YouTube Playlist"},
    "yt.url_label":         {"ar": "رابط البلاي ليست:", "en": "Playlist URL:"},
    "yt.url_placeholder":   {"ar": "https://www.youtube.com/playlist?list=…",
                             "en": "https://www.youtube.com/playlist?list=…"},
    "yt.fetch":             {"ar": "جلب الترتيب", "en": "Fetch order"},
    "yt.fetching":          {"ar": "جارٍ جلب البلاي ليست…", "en": "Fetching the playlist…"},
    "yt.apply":             {"ar": "تطبيق الترتيب", "en": "Apply order"},
    "yt.cancel":            {"ar": "إلغاء", "en": "Cancel"},
    "yt.need_ytdlp":        {"ar": "مكتبة yt-dlp مش مثبتة عندي. ثبّتها بالأمر:\n"
                                   "pip install yt-dlp",
                             "en": "The yt-dlp library isn't installed. Install it with:\npip install yt-dlp"},
    "yt.error":             {"ar": "تعذر جلب البلاي ليست:\n{err}",
                             "en": "Couldn't fetch the playlist:\n{err}"},
    "yt.empty":             {"ar": "البلاي ليست فاضية — أو اللينك مش لبلاي ليست.",
                             "en": "The playlist is empty — or the link isn't a playlist."},
    "yt.matched":           {"ar": "تمت مطابقة {n} فيديو وترتيبها زي يوتيوب بالظبط ✔",
                             "en": "Matched {n} videos and ordered them exactly like YouTube ✔"},
    "yt.unmatched_files":   {"ar": "ملفات محلية مالهاش مقابل في البلاي ليست (فضلت في مكانها): {n}",
                             "en": "Local files with no playlist match (left in place): {n}"},
    "yt.missing_entries":   {"ar": "فيديوهات في البلاي ليست مش موجودة عندك محلّيًا: {n}",
                             "en": "Playlist videos not present locally: {n}"},
    "yt.hint":              {"ar": "التطابق بيحصل بالاسم — لو أسماء الملفات مختلفة كليًا عن "
                                   "أسماء فيديوهات اليوتيوب، الترتيب مش هيتغير.",
                             "en": "Matching is by title — if your filenames differ completely "
                                   "from the YouTube titles, the order won't change."},

    # ---------- tray ----------
    "tray.tooltip":         {"ar": "Timeline Videos — شغال في الخلفية",
                             "en": "Timeline Videos — running in the tray"},
    "tray.show":            {"ar": "إظهار {app}", "en": "Show {app}"},
    "tray.exit":            {"ar": "خروج", "en": "Exit"},
    "tray.still_running":   {"ar": " البرنامج ما زال شغّال في شريط النظام.",
                             "en": " Still running in the tray."},
}


def set_language(lang: str):
    global _current_lang
    _current_lang = lang if lang in (LANG_AR, LANG_EN) else _DEFAULT_LANG


def get_language() -> str:
    return _current_lang


def is_rtl() -> bool:
    return _current_lang == LANG_AR


def tr(key: str, **kwargs) -> str:
    """Translate a key; falls back to the key itself if missing."""
    entry = _STRINGS.get(key)
    if entry is None:
        return key
    text = entry.get(_current_lang) or entry.get(LANG_EN) or key
    if kwargs:
        try:
            text = text.format(**kwargs)
        except (KeyError, IndexError):
            pass
    return text


# ---------- shared value formatting ----------

def format_duration(seconds: float) -> str:
    """125.0 -> '2 د 5 ث' (ar) / '2m 5s' (en);  7500 -> '2 س 5 د' / '2h 5m'."""
    seconds = int(round(seconds or 0))
    if seconds < 0:
        seconds = 0
    h, rem = divmod(seconds, 3600)
    m, s = divmod(rem, 60)
    if _current_lang == LANG_AR:
        if h:
            return f"{h} س {m} د"
        if m:
            return f"{m} د {s} ث"
        return f"{s} ث"
    if h:
        return f"{h}h {m}m"
    if m:
        return f"{m}m {s}s"
    return f"{s}s"


def format_size(num_bytes: int) -> str:
    """Human file size, Western digits + local unit names."""
    size = float(num_bytes or 0)
    if _current_lang == LANG_AR:
        units = ("ب", "ك.ب", "م.ب", "ج.ب", "ت.ب")
    else:
        units = ("B", "KB", "MB", "GB", "TB")
    idx = 0
    while size >= 1024 and idx < len(units) - 1:
        size /= 1024.0
        idx += 1
    if idx == 0:
        return f"{int(size)} {units[idx]}"
    return f"{size:.1f} {units[idx]}"
