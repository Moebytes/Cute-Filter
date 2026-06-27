#include "Processor.h"
#include "Editor.h"
#include "Functions.hpp"
#include "Settings.hpp"
#include "NativeMenuBridge.h"
#include "BinaryData.h"

Editor::Editor(Processor& p) : AudioProcessorEditor(&p), processor(p),
    webview(webviewOptions()) {

    webview.goToURL(webview.getResourceProviderRoot());

    int width = static_cast<int>(Settings::getSettingKey("windowWidth", 600));
    int height = static_cast<int>(Settings::getSettingKey("windowHeight", 520));
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    int minWidth = 240;
    int minHeight = static_cast<int>(static_cast<float>(minWidth) / aspectRatio); 

    constrainer.setFixedAspectRatio(aspectRatio);
    constrainer.setMinimumSize(minWidth, minHeight);
    constrainer.setMaximumSize(10000, 10000);
    
    this->setConstrainer(&constrainer);
    this->setResizable(true, true);
    this->setSize(width, height);

    this->addAndMakeVisible(webview);

    EventEmitter::instance().addListener(this);

    this->startTimerHz(30);
}

Editor::~Editor() {
    EventEmitter::instance().removeListener(this);
}

auto Editor::webviewOptions() -> WebBrowserComponent::Options {
    return WebBrowserComponent::Options{}
    .withBackend(WebBrowserComponent::Options::Backend::webview2)
    .withWinWebView2Options(WebBrowserComponent::Options::WinWebView2{}
    .withUserDataFolder(File::getSpecialLocation(File::tempDirectory)))
    .withResourceProvider([this](const auto& url) { return getResource(url); })
    .withNativeIntegrationEnabled()
    .withKeepPageLoadedWhenBrowserIsHidden()
    .withOptionsFrom(filterRelay)
    .withOptionsFrom(frequencyRelay)
    .withOptionsFrom(resonanceRelay)
    .withOptionsFrom(slopeRelay)
    .withOptionsFrom(filterLFOTypeRelay)
    .withOptionsFrom(filterLFORateRelay)
    .withOptionsFrom(filterLFOAmountRelay)
    .withOptionsFrom(filterLFOInvertRelay)
    .withNativeFunction("getDefaultParameter", [this](auto args, auto completion){ 
        return this->processor.parameters.getDefaultParameter(args, completion); 
    })
    .withNativeFunction("openFilterSelector", [this](auto args, auto completion){ 
        return this->openFilterSelector(args, completion); 
    })
    .withNativeFunction("openPresetMenu", [this](auto args, auto completion){ 
        return this->processor.presetManager.openPresetMenu(args, completion); 
    })
    .withNativeFunction("prevPreset", [this](auto args, auto completion){ 
        return this->processor.presetManager.prevPreset(args, completion); 
    })
    .withNativeFunction("nextPreset", [this](auto args, auto completion){ 
        return this->processor.presetManager.nextPreset(args, completion); 
    })
    .withNativeFunction("currentPresetName", [this]([[maybe_unused]] auto args, auto completion){ 
        return completion(this->processor.presetManager.currentPresetName);
    })
    .withEventListener("backendReady", [this]([[maybe_unused]] auto obj) {
        this->webviewBackendReady = true;
    })
    .withEventListener("themeChange", [this](auto obj) {
        if (auto* data = obj.getDynamicObject()) {
            auto theme = data->getProperty("theme").toString();
            this->handleThemeChange(theme);
        }
    });
}

auto Editor::resized() -> void {
    webview.setBounds(getLocalBounds());
    Settings::setSettingKey("windowWidth", getWidth());
    Settings::setSettingKey("windowHeight", getHeight());
}

auto Editor::getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte> {
    MemoryInputStream zipStream(BinaryData::webview_files_zip, BinaryData::webview_files_zipSize, false);
    ZipFile zip{zipStream};

    if (auto* entry = zip.getEntry(resourceStr)) {
        std::unique_ptr<InputStream> entryStream{zip.createStreamForEntry(*entry)};
        if (entryStream == nullptr) {
            jassertfalse;
            return {};
        }
        return Functions::streamToVector(*entryStream);
    }
    return {};
}

auto Editor::getResource(const String& url) -> std::optional<WebBrowserComponent::Resource> {
    static auto fileRoot = File::getCurrentWorkingDirectory().getChildFile("dist");
    auto resourceStr = url == "/" ? "index.html" : url.fromFirstOccurrenceOf("/", false, false);
    auto ext = resourceStr.fromLastOccurrenceOf(".", false, false);

    #if WEBVIEW_DEV_MODE
        auto stream = fileRoot.getChildFile(resourceStr).createInputStream();
        if (stream) {
            return WebBrowserComponent::Resource(Functions::streamToVector(*stream), Functions::getMimeForExtension(ext));
        }
    #else
        auto resource = Editor::getWebviewFileBytes(resourceStr);
        if (!resource.empty()) {
            return WebBrowserComponent::Resource(std::move(resource), Functions::getMimeForExtension(ext));
        }
    #endif
    return std::nullopt;
}

auto Editor::handleThemeChange(const String& theme) -> void {
    this->processor.presetManager.presetDialogTheme.setTheme(theme == "dark");

    if (this->processor.presetManager.presetDialog != nullptr) {
        this->processor.presetManager.presetDialog->sendLookAndFeelChange();

        this->processor.presetManager.presetDialog->getTextEditor("name")
            ->applyColourToAllText(theme == "dark" ? Colours::white : Colours::black);

        this->processor.presetManager.presetDialog->getTextEditor("author")
            ->applyColourToAllText(theme == "dark" ? Colours::white : Colours::black);
    }
}

auto Editor::handleEvent(const String& name, const var& payload) -> void {
    if (name == "presetChanged") {
        this->webview.emitEventIfBrowserIsVisible(Identifier{name}, payload.toString());
    }
}

auto Editor::timerCallback() -> void {
    if (!this->webviewBackendReady) return;
    if (!this->webview.isShowing()) return;

    this->processor.updateFilterResponse();

    var arr;
    for (float x : this->processor.filterResponse) {
        arr.append(x);
    }
    
    this->webview.emitEventIfBrowserIsVisible(Identifier{"filterResponse"}, arr);
}

auto Editor::openFilterSelector([[maybe_unused]] const Array<var>& args, 
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {

    std::map<int, std::string> items = {
        {1, "Lowpass 12 dB"},
        {2, "Lowpass 24 dB"},
        {3, "Lowpass 36 dB"},
        {4, "Lowpass 48 dB"},
        {5, "Highpass 12 dB"},
        {6, "Highpass 24 dB"},
        {7, "Highpass 36 dB"},
        {8, "Highpass 48 dB"},
        {9, "Bandpass 12 dB"},
        {10, "Bandpass 24 dB"},
        {11, "Bandpass 36 dB"},
        {12, "Bandpass 48 dB"}
    };

    auto menuClick = [this](std::string action) {
        auto* filterParam = processor.parameters.filterParam;
        auto* slopeParam = processor.parameters.slopeParam;

        int filterIndex = 0;
        int slopeIndex = 0;

        if (action == "Lowpass 12 dB") {
            filterIndex = filterParam->choices.indexOf("lowpass");
            slopeIndex = slopeParam->choices.indexOf("12 dB");
        } else if (action == "Lowpass 24 dB") {
            filterIndex = filterParam->choices.indexOf("lowpass");
            slopeIndex = slopeParam->choices.indexOf("24 dB");
        } else if (action == "Lowpass 36 dB") {
            filterIndex = filterParam->choices.indexOf("lowpass");
            slopeIndex = slopeParam->choices.indexOf("36 dB");
        } else if (action == "Lowpass 48 dB") {
            filterIndex = filterParam->choices.indexOf("lowpass");
            slopeIndex = slopeParam->choices.indexOf("48 dB");

        } else if (action == "Highpass 12 dB") {
            filterIndex = filterParam->choices.indexOf("highpass");
            slopeIndex = slopeParam->choices.indexOf("12 dB");
        } else if (action == "Highpass 24 dB") {
            filterIndex = filterParam->choices.indexOf("highpass");
            slopeIndex = slopeParam->choices.indexOf("24 dB");
        } else if (action == "Highpass 36 dB") {
            filterIndex = filterParam->choices.indexOf("highpass");
            slopeIndex = slopeParam->choices.indexOf("36 dB");
        } else if (action == "Highpass 48 dB") {
            filterIndex = filterParam->choices.indexOf("highpass");
            slopeIndex = slopeParam->choices.indexOf("48 dB");

        } else if (action == "Bandpass 12 dB") {
            filterIndex = filterParam->choices.indexOf("bandpass");
            slopeIndex = slopeParam->choices.indexOf("12 dB");
        } else if (action == "Bandpass 24 dB") {
            filterIndex = filterParam->choices.indexOf("bandpass");
            slopeIndex = slopeParam->choices.indexOf("24 dB");
        } else if (action == "Bandpass 36 dB") {
            filterIndex = filterParam->choices.indexOf("bandpass");
            slopeIndex = slopeParam->choices.indexOf("36 dB");
        } else if (action == "Bandpass 48 dB") {
            filterIndex = filterParam->choices.indexOf("bandpass");
            slopeIndex = slopeParam->choices.indexOf("48 dB");
        }

        filterParam->setValueNotifyingHost(filterParam->convertTo0to1(static_cast<float>(filterIndex)));
        slopeParam->setValueNotifyingHost(slopeParam->convertTo0to1(static_cast<float>(slopeIndex)));
    };

    #if JUCE_MAC
        showNativeMacMenu(items, std::map<int, std::string>{}, std::map<int, std::string>{}, 
            "", "", "", false, [items, menuClick](int resultID) {
            if (resultID == 0) return;
            
            auto it = items.find(resultID);
            if (it != items.end()) {
                menuClick(it->second);
            }
        });
    #elif _WIN32
        showNativeWinMenu(items, std::map<int, std::string>{}, std::map<int, std::string>{}, 
            "", "", "", false, [items, menuClick](int resultID) {
            if (resultID == 0) return;
            
            auto it = items.find(resultID);
            if (it != items.end()) {
                menuClick(it->second);
            }
        });
    #else
        PopupMenu menu;
        for (const auto& [id, label] : items) {
            menu.addItem(id, label);
        }

        auto mousePos = Desktop::getMousePosition();
        
        auto options = PopupMenu::Options()
            .withPreferredPopupDirection(PopupMenu::Options::PopupDirection::upwards)
            .withTargetScreenArea(Rectangle<int> {mousePos.x - 60, mousePos.y - 5, 1, 1});

        PopupMenu::dismissAllActiveMenus();
        menu.showMenuAsync(options, [items, menuClick](int resultID) {
            if (resultID == 0) return;
            
            auto it = items.find(resultID);
            if (it != items.end()) {
                menuClick(it->second);
            }
        });
    #endif
}