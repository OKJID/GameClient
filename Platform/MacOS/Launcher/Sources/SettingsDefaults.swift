import Foundation

struct SettingsDefaults {
    static let isWindowedEdgeScrollEnabled = false
    
    static let cameraMinHeight: Double = 210.0
    static let cameraMinHeightRange: ClosedRange<Double> = 100...300
    static let cameraMinHeightStep: Double = 1.0
    static let cameraMinHeightFormat = "%.0f"
    
    static let cameraMaxHeight: Double = 310.0
    static let cameraMaxHeightRange: ClosedRange<Double> = 310...600
    static let cameraMaxHeightStep: Double = 1.0
    static let cameraMaxHeightFormat = "%.0f"
    
    static let cameraMoveSpeed: Double = 1.0
    static let cameraMoveSpeedRange: ClosedRange<Double> = 0.5...3.0
    static let cameraMoveSpeedStep: Double = 0.1
    static let cameraMoveSpeedFormat = "%.1f"
    
    static let limitFramerate = true
    
    static let fpsLimit: Double = 60.0
    static let fpsLimitRange: ClosedRange<Double> = 30...240
    static let fpsLimitStep: Double = 5.0
    static let fpsLimitFormat = "%.0f"
    
    static let statsOverlay = false
    static let useAlternativeEndpoint = false
    static let verboseLogging = false
}
