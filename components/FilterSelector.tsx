import React, {useState, useEffect} from "react"
import * as JUCE from "juce-framework-frontend-mirror"
import parameters from "../processor/parameters.json"
import "./styles/filterselector.scss"

const openFilterSelector = JUCE.getNativeFunction("openFilterSelector")

const FilterSelector: React.FunctionComponent = () => {
    const filterState = JUCE.getComboBoxState(parameters.filter.id)!
    const filterChoices = filterState.properties.choices
    
    const slopeState = JUCE.getComboBoxState(parameters.slope.id)!
    const slopeChoices = slopeState.properties.choices

    const [filterIndex, setFilterIndex] = useState(filterState.getChoiceIndex())
    const [slopeIndex, setSlopeIndex] = useState(slopeState.getChoiceIndex())

    useEffect(() => {
        setFilterIndex(filterState.getChoiceIndex())
        setSlopeIndex(slopeState.getChoiceIndex())
    }, [])

    useEffect(() => {
        const filterValueID = filterState.valueChangedEvent.addListener(() => {
            setFilterIndex(filterState.getChoiceIndex())
        })
        const slopeValueID = slopeState.valueChangedEvent.addListener(() => {
            setSlopeIndex(slopeState.getChoiceIndex())
        })
        return () => {
            filterState.valueChangedEvent.removeListener(filterValueID)
            slopeState.valueChangedEvent.removeListener(slopeValueID)
        }
    }, [])

    const filterSelector = async () => {
        await openFilterSelector()
    }

    const filterString = () => {
        const filter = filterChoices?.[filterIndex]
        const slope = slopeChoices?.[slopeIndex]

        if (!filter || !slope) return "LOWPASS 24 dB"

        return `${String(filter)?.toUpperCase()} ${slope}`
    }

    return (
        <div className="filter-selector" onPointerDown={filterSelector}>
            <span className="filter-selector-label">
                {filterString()}
            </span>
        </div>
    )
}

export default FilterSelector