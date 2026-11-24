//! Community EXE-only signed metadata. Not yet connected to the live updater.
use super::{compare_versions, hash_value, parse_version, positive_size, verify_signed_payload};
use crate::error::{Result, UpdateError, require};
use serde_json::Value;
use std::cmp::Ordering;
use time::OffsetDateTime;
use time::format_description::well_known::Rfc3339;

pub const FEED_NAME: &str = "community-installers.json";
pub const REPOSITORY: &str = "wxyu71/snow-shot";

#[derive(Clone, Debug)]
pub struct Installer {
    pub variant: String,
    pub name: String,
    pub size: u64,
    pub sha256: String,
}

#[derive(Clone, Debug)]
pub struct InstallerRelease {
    pub version: String,
    pub installers: Vec<Installer>,
}

fn invalid() -> UpdateError {
    UpdateError::new("invalid_community_installer_release", "Invalid Community installer release")
}

impl InstallerRelease {
    pub fn installer(&self, variant: &str) -> Result<&Installer> {
        self.installers.iter().find(|p| p.variant == variant).ok_or_else(invalid)
    }
}

pub fn verify(
    envelope: &[u8],
    trusted_keys: Option<&[u8]>,
    current_version: &str,
    tag: &str,
) -> Result<InstallerRelease> {
    require(!crate::edition::MINI && crate::edition::PLATFORM == "windows-x64",
        "unsupported_update_release", "Unsupported Community installation")?;
    let payload = verify_signed_payload(envelope, trusted_keys)?;
    let root: Value = serde_json::from_slice(&payload).map_err(|_| invalid())?;
    let object = root.as_object().ok_or_else(invalid)?;
    let version = object.get("version").and_then(Value::as_str).ok_or_else(invalid)?;
    let parsed = parse_version(version)?;
    let beta = parsed.pre.as_str().strip_prefix("beta.").is_some_and(|n|
        !n.is_empty() && n.bytes().all(|b| b.is_ascii_digit()));
    require(object.len() == 7
        && root["schema"] == 2
        && root["product"] == "snow-shot-community"
        && root["repository"] == REPOSITORY
        && root["platform"] == "windows-x64"
        && root["publishedAt"].as_str().is_some_and(|s| OffsetDateTime::parse(s, &Rfc3339).is_ok())
        && beta && parsed.build.is_empty()
        && tag == format!("v{version}_snow-shot")
        && compare_versions(version, current_version)? == Ordering::Greater,
        "invalid_community_installer_release", "Invalid Community installer release")?;
    let list = root["installers"].as_array().ok_or_else(invalid)?;
    require(list.len() == 2, "invalid_community_installer_release", "Expected two Community installers")?;
    let mut installers = Vec::with_capacity(2);
    for item in list {
        let fields = item.as_object().ok_or_else(invalid)?;
        let variant = item["variant"].as_str().ok_or_else(invalid)?;
        let name = item["name"].as_str().ok_or_else(invalid)?;
        require(fields.len() == 4
            && matches!(variant, "online" | "offline")
            && name == format!("snow-shot-{version}-windows-x64-{variant}.exe")
            && !installers.iter().any(|p: &Installer| p.variant == variant),
            "invalid_community_installer_release", "Invalid Community installer identity")?;
        installers.push(Installer {
            variant: variant.to_owned(), name: name.to_owned(),
            size: positive_size(fields.get("size"), false)?,
            sha256: hash_value(fields.get("sha256"))?,
        });
    }
    Ok(InstallerRelease { version: version.to_owned(), installers })
}

#[cfg(all(test, not(feature = "mini"), not(target_arch = "aarch64")))]
mod tests {
    use super::*;
    use crate::contract::tests::{sign_payload, trusted_key};
    use rand::rngs::OsRng;
    use rsa::RsaPrivateKey;
    use serde_json::json;

    #[test]
    fn accepts_only_signed_newer_exact_community_installers() {
        let key = RsaPrivateKey::new(&mut OsRng, 3072).unwrap();
        let trust = trusted_key(&key, None);
        let payload = json!({
            "schema": 2, "product": "snow-shot-community", "repository": REPOSITORY,
            "platform": "windows-x64", "publishedAt": "2026-11-24T12:00:00Z",
            "version": "1.6.3-beta.2",
            "installers": [
                {"variant":"online","name":"snow-shot-1.6.3-beta.2-windows-x64-online.exe","size":1,"sha256":"a".repeat(64)},
                {"variant":"offline","name":"snow-shot-1.6.3-beta.2-windows-x64-offline.exe","size":2,"sha256":"b".repeat(64)}
            ]
        });
        let tag = "v1.6.3-beta.2_snow-shot";
        let signed = sign_payload(&payload, &key, 32);
        let release = verify(&signed, Some(&trust), "1.6.3-beta.1", tag).unwrap();
        assert_eq!(release.installer("offline").unwrap().size, 2);
        assert!(release.installer("portable").is_err());
        assert!(verify(&signed, None, "1.6.3-beta.1", tag).is_err());
        assert!(verify(&signed, Some(&trust), "1.6.3-beta.2", tag).is_err());
        assert!(verify(&signed, Some(&trust), "1.6.4-beta.1", tag).is_err());
        assert!(verify(&signed, Some(&trust), "1.6.3-beta.1", "v1.6.3-beta.3_snow-shot").is_err());
        assert!(verify(&serde_json::to_vec(&payload).unwrap(), Some(&trust), "1.6.3-beta.1", tag).is_err());
        for (field, value) in [("repository", json!("other/snow-shot")), ("platform", json!("windows-arm64")), ("product", json!("snow-shot-mini")), ("schema", json!(1))] {
            let mut changed = payload.clone();
            changed[field] = value;
            assert!(verify(&sign_payload(&changed, &key, 32), Some(&trust), "1.6.3-beta.1", tag).is_err());
        }
        for (field, value) in [("name", json!("../bad.exe")), ("size", json!(0)), ("sha256", json!("bad")), ("variant", json!("offline"))] {
            let mut changed = payload.clone();
            changed["installers"][0][field] = value;
            assert!(verify(&sign_payload(&changed, &key, 32), Some(&trust), "1.6.3-beta.1", tag).is_err());
        }
    }
}
