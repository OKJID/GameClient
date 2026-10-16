// Ed25519 keys and signatures for the launcher self-update feed.
//
// Run (from Platform/MacOS/Launcher/):
//   swift update_signing.swift keygen
//   GO_UPDATE_PRIVATE_KEY=... swift update_signing.swift sign <version> <build> <sha256>
//   swift update_signing.swift verify <version> <build> <sha256> <signature> <public key>
//
// The signed message must stay byte-identical to UpdateSignature.message in
// Sources/LauncherUpdater.swift, or no launcher accepts the feed.

import CryptoKit
import Foundation

func signedMessage(version: String, build: String, sha256: String) -> Data {
    Data("\(version)\n\(build)\n\(sha256.lowercased())".utf8)
}

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("\(message)\n".utf8))
    exit(1)
}

func keygen() {
    let key = Curve25519.Signing.PrivateKey()
    print("GO_UPDATE_PRIVATE_KEY=\(key.rawRepresentation.base64EncodedString())")
    print("GO_UPDATE_PUBLIC_KEY=\(key.publicKey.rawRepresentation.base64EncodedString())")
}

func sign(_ arguments: [String]) {
    guard arguments.count == 3 else {
        fail("usage: sign <version> <build> <sha256>")
    }

    guard let encoded = ProcessInfo.processInfo.environment["GO_UPDATE_PRIVATE_KEY"],
          let raw = Data(base64Encoded: encoded),
          let key = try? Curve25519.Signing.PrivateKey(rawRepresentation: raw) else {
        fail("GO_UPDATE_PRIVATE_KEY is missing or malformed")
    }

    let message = signedMessage(version: arguments[0], build: arguments[1], sha256: arguments[2])
    guard let signature = try? key.signature(for: message) else {
        fail("signing failed")
    }

    print(signature.base64EncodedString())
}

func verify(_ arguments: [String]) {
    guard arguments.count == 5 else {
        fail("usage: verify <version> <build> <sha256> <signature> <public key>")
    }

    guard let signature = Data(base64Encoded: arguments[3]),
          let rawKey = Data(base64Encoded: arguments[4]),
          let key = try? Curve25519.Signing.PublicKey(rawRepresentation: rawKey) else {
        fail("signature or public key is malformed")
    }

    let message = signedMessage(version: arguments[0], build: arguments[1], sha256: arguments[2])
    guard key.isValidSignature(signature, for: message) else {
        fail("signature does not match the public key")
    }

    print("signature ok")
}

let arguments = Array(CommandLine.arguments.dropFirst())

switch arguments.first {
case "keygen":
    keygen()
case "sign":
    sign(Array(arguments.dropFirst()))
case "verify":
    verify(Array(arguments.dropFirst()))
default:
    fail("usage: keygen | sign <version> <build> <sha256> | verify <version> <build> <sha256> <signature> <public key>")
}
