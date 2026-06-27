import React, {useState, useEffect, useMemo} from "react"
import "./styles/filtervisualizer.scss"

const width = 300
const height = 120

const FilterVisualizer: React.FunctionComponent = () => {
    const [response, setResponse] = useState<number[]>([])

    useEffect(() => {
        const eventID = window.__JUCE__.backend.addEventListener("filterResponse", updateResponse)

        return () => {
            window.__JUCE__.backend.removeEventListener(eventID)
        }
    }, [])

    const updateResponse = (response: number[]) => {
        setResponse(response)
    }

    const path = useMemo(() => {
        if (response.length === 0) return ""

        const points = response.map((magnitude, i) => {
            const x = (i / (response.length - 1)) * width

            const db = 20 * Math.log10(Math.max(magnitude, 1e-6))
            const norm = Math.max(0, Math.min(1, (db + 60) / 72))

            const y = height * (1 - norm)

            return {x, y}
        })

        let d = `M ${points[0].x} ${points[0].y}`

        for (let i = 1; i < points.length; i++) {
            d += ` L ${points[i].x} ${points[i].y}`
        }

        return d
    }, [response])

    return (
        <div className="filter-visualizer">
            <svg
                width={width}
                height={height}
                viewBox={`0 0 ${width} ${height}`}>
                <path
                    d={path}
                    fill="none"
                    stroke="white"
                    strokeWidth={2}
                />
            </svg>
        </div>
    )
}

export default FilterVisualizer