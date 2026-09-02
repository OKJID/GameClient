import Foundation

struct DonationMethod: Identifiable {
    let id: String
    let title: String
    let icon: String
    let url: String
    let subtitle: () -> String
}

enum Donations {
    static let methods: [DonationMethod] = [
        DonationMethod(
            id: "kofi",
            title: "Ko-fi",
            icon: "cup.and.saucer.fill",
            url: "https://ko-fi.com/okjid",
            subtitle: { L10n.donate.cardDesc }
        ),
        DonationMethod(
            id: "paypal",
            title: "PayPal",
            icon: "creditcard.fill",
            url: "https://www.paypal.com/donate/?hosted_button_id=9D3X8N5JRRHTA",
            subtitle: { L10n.donate.paypalDesc }
        )
    ]
}
