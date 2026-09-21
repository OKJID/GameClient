import Foundation
import Network

protocol ApiResource: AnyObject {
    var resourceName: String { get }

    func reload() throws
}

enum ApiSyncStatus {
    case initializing
    case current
    case recent
    case unverified
}

final class ApiSync: ObservableObject {
    private static let baseURL = URL(string: "https://general-online-zh.web.app/api/")!
    private static let tickInterval: TimeInterval = 5 * 60
    private static let requestTimeout: TimeInterval = 10
    private static let freshnessWindow: TimeInterval = 60 * 60

    let resources: [ApiResource]

    @Published private(set) var status: ApiSyncStatus = .initializing
    @Published private(set) var isDiskAccessDenied = false

    private let monitor = NWPathMonitor()
    private var tickTimer: Timer?
    private var expiryTimer: Timer?
    private var isTicking = false
    private var isOnline = false
    private var lastTickSucceeded: Bool?
    private var lastSuccessAt: Date?

    init(resources: [ApiResource]) {
        self.resources = resources
    }

    private var resolvedStatus: ApiSyncStatus {
        guard let lastTickSucceeded else { return .initializing }
        guard let lastSuccessAt, Date().timeIntervalSince(lastSuccessAt) < Self.freshnessWindow else {
            return .unverified
        }

        return lastTickSucceeded ? .current : .recent
    }

    func start() {
        resources.forEach(reload)
        startMonitor()

        tickTimer = Timer.scheduledTimer(withTimeInterval: Self.tickInterval, repeats: true) { [weak self] _ in
            self?.tick()
        }

        tick()
    }

    func tick() {
        guard !isTicking else { return }
        isTicking = true

        let group = DispatchGroup()
        var failedCount = 0

        for resource in resources {
            group.enter()
            fetch(resource) { succeeded in
                if !succeeded {
                    failedCount += 1
                }

                group.leave()
            }
        }

        group.notify(queue: .main) { [weak self] in
            self?.finishTick(succeeded: failedCount == 0)
        }
    }

    private func fetch(_ resource: ApiResource, completion: @escaping (Bool) -> Void) {
        var request = URLRequest(url: Self.baseURL.appendingPathComponent(resource.resourceName))
        request.cachePolicy = .reloadIgnoringLocalAndRemoteCacheData
        request.timeoutInterval = Self.requestTimeout

        URLSession.shared.dataTask(with: request) { [weak self] data, response, error in
            let statusCode = (response as? HTTPURLResponse)?.statusCode

            DispatchQueue.main.async {
                guard let self, error == nil, statusCode == 200, let data else {
                    completion(false)
                    return
                }

                completion(self.store(data, for: resource))
            }
        }.resume()
    }

    private func store(_ data: Data, for resource: ApiResource) -> Bool {
        do {
            try ApiCache.write(data, as: resource.resourceName)
        } catch {
            isDiskAccessDenied = true
            return false
        }

        reload(resource)
        return true
    }

    private func reload(_ resource: ApiResource) {
        do {
            try resource.reload()
        } catch {
            isDiskAccessDenied = true
        }
    }

    private func finishTick(succeeded: Bool) {
        isTicking = false
        lastTickSucceeded = succeeded

        if succeeded {
            lastSuccessAt = Date()
            scheduleExpiry()
        }

        status = resolvedStatus
    }

    private func scheduleExpiry() {
        expiryTimer?.invalidate()
        expiryTimer = Timer.scheduledTimer(withTimeInterval: Self.freshnessWindow, repeats: false) { [weak self] _ in
            guard let self else { return }
            self.status = self.resolvedStatus
        }
    }

    private func startMonitor() {
        monitor.pathUpdateHandler = { [weak self] path in
            let isSatisfied = path.status == .satisfied

            DispatchQueue.main.async {
                self?.handlePath(isSatisfied: isSatisfied)
            }
        }

        monitor.start(queue: DispatchQueue(label: "ApiSync.monitor"))
    }

    private func handlePath(isSatisfied: Bool) {
        let cameOnline = isSatisfied && !isOnline
        isOnline = isSatisfied

        guard cameOnline else { return }
        tick()
    }
}
