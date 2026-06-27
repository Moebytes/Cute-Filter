import React, {useState, useEffect} from "react"
import {createRoot} from "react-dom/client"
import DarkIcon from "./assets/dark.svg"
import LightIcon from "./assets/light.svg"
import Knob from "./components/Knob"
import SkewKnob from "./components/SkewKnob"
import FilterType from "./components/FilterType"
import FilterSlope from "./components/FilterSlope"
import FilterSelector from "./components/FilterSelector"
import FilterVisualizer from "./components/FilterVisualizer"
import LFOBar from "./components/LFOBar"
import PresetBar from "./components/PresetBar"
import parameters from "./processor/parameters.json"
import functions from "./structures/Functions"
import "./index.scss"

const darkColorList = {
    "--background": "#220E1A",
    "--textColor": "#FFFFFF"
}

const lightColorList = {
    "--background": "#FFBEE6",
    "--textColor": "#000000",
}

type ThemeContextType = {theme: string; setTheme: React.Dispatch<React.SetStateAction<string>>}
export const ThemeContext = React.createContext<ThemeContextType>({theme: "", setTheme: () => null})

const App: React.FunctionComponent = () => {
    const [theme, setTheme] = useState(localStorage.getItem("theme") || "light")

    useEffect(() => {
        window.__JUCE__.backend.emitEvent("themeChange", {theme})
        window.__JUCE__.backend.emitEvent("backendReady", true)
    }, [])

    useEffect(() => {
        const colorList = theme === "light" ? lightColorList : darkColorList
        for (const [key, color] of Object.entries(colorList)) {
            document.documentElement.style.setProperty(key, color)
        }
        localStorage.setItem("theme", theme)
        window.__JUCE__.backend.emitEvent("themeChange", {theme})
    }, [theme])

    const toggleTheme = () => {
        setTheme((prev) => prev === "light" ? "dark" : "light")
    }

    const filter = functions.calculateFilter("#FF4EA7")

    return (
        <div className="app">
            <ThemeContext.Provider value={{theme, setTheme}}>
            <div className="title-container">
                <span className="title-text">Cute Filter</span>
                {theme === "light" ? 
                <DarkIcon className="theme-icon" style={{filter}} onClick={toggleTheme}/> :
                <LightIcon className="theme-icon" style={{filter}} onClick={toggleTheme}/>}
            </div>

            <div className="knobs-container">
                <SkewKnob 
                    label={"FREQ"} 
                    parameterID={parameters.frequency.id} 
                    color="#FF4EA7" 
                    display="hz"/>
                <Knob 
                    label={"RES"} 
                    parameterID={parameters.resonance.id} 
                    color="#FF4EA7" 
                    display="q"/>
                <div className="filter-container">
                    {/*<FilterType
                        parameterID={parameters.filter.id}/>
                    <FilterSlope
                        parameterID={parameters.slope.id}/>*/}
                    <FilterSelector/>
                    <FilterVisualizer/>
                </div>
            </div>
            <div className="lfo-container">
                <LFOBar 
                    label={parameters.filter.id.toUpperCase()}
                    lfoTypeID={parameters.filterLFOType.id} 
                    lfoRateID={parameters.filterLFORate.id} 
                    lfoAmountID={parameters.filterLFOAmount.id} 
                    lfoInvertID={parameters.filterLFOInvert.id}
                    color="#FF4EA7"
                    theme={theme}/>
            </div>
            <div className="preset-container">
                <PresetBar/>
            </div>
            </ThemeContext.Provider>
        </div>
    )

}

const root = createRoot(document.getElementById("root")!)
root.render(<App/>)