# CSLOL DLL License Addendum (Distribution & Use Policy)

**Version:** 1.1  
**Applies to:** `cslol-dll.dll` and Derivative Binaries (as defined below)  
**Licensor:** League Toolkit Organization (“Licensor”)  
**Effective date:** 2026-01-01

## 1. Purpose

This Addendum governs the **distribution and use** of the core binary module commonly named **`cslol-dll.dll`** (the “DLL”), which enables the host software to inject League of Legends modifications (“Mods”).

The intent of this Addendum is to:
1) ensure accountability of redistributors through public-trust code signing, and  
2) require technical enforcement against prohibited content (“Skinhacking,” as defined below).

## 2. Relationship to the Project License

The broader CSLOL project may be distributed under **GPL-3.0** (or another project-wide license). **Notwithstanding any project-wide license, this Addendum solely governs the DLL and Derivative Binaries.** In the event of a conflict between this Addendum and any project-wide license, **this Addendum controls** for the DLL and Derivative Binaries.

## 3. Definitions

3.1 **“DLL”** means the binary file `cslol-dll.dll` distributed by Licensor.

3.2 **“Derivative Binary”** means any modified, patched, rebuilt, forked, recompiled, or otherwise altered version of the DLL, including any binary that is substantially based on the DLL’s code or functionality.

3.3 **“Distributor”** means any person or entity that Distributes the DLL or a Derivative Binary.

3.4 **“Distribute” / “Distribution”** means to copy, share, publish, host, bundle, ship, provide for download, or otherwise make the DLL or a Derivative Binary available to any third party by any means, including (without limitation) hosting on Git platforms, mirrors, CDNs, release pages, installers, auto-updaters, or embedding/bundling within third-party software. For clarity, **using the DLL in custom third-party launcher software is considered Distribution if the DLL is provided to users or third parties.**

3.5 **“Launcher”** means any third-party software that loads, bundles, installs, manages, or invokes the DLL to install or inject Mods.

3.6 **“Skins / Paid Content”** means purchasable or previously purchasable League of Legends cosmetic content, including content reflected in PBE or live manifests.

3.7 **“Skinhacking”** means any Prohibited Content described in Section 6.

## 4. Limited Permission

Subject to continuous compliance with this Addendum, Licensor grants Distributor a limited, revocable, non-exclusive permission to:

- Distribute **unmodified** copies of the DLL, and/or
- Distribute Launchers that use the DLL,

**provided that** all conditions in Sections 5–8 are met.

No other rights are granted.

## 5. Code Signing Requirements (Mandatory)

5.1 **No Licensor Signature Redistribution.** Distributor must not Distribute any copy of the DLL or Derivative Binary that bears **Licensor’s original code signing signature**.

5.2 **Re-signing Required.** Any party Distributing the DLL must re-sign the DLL using a **publicly trusted code-signing certificate** issued by a reputable, trusted Certificate Authority (e.g., DigiCert, Certum, or comparable).

5.3 **Timestamp Required.** The signature must include a **trusted timestamp** (RFC 3161 or equivalent) from a trusted timestamp authority.

5.4 **Certificate Identity.** The signing certificate must identify the Distributor (organization or individual) in a manner consistent with the CA’s validation level, so that responsibility for Distribution is attributable.

5.5 **Verification & Transparency (Required).** To support verification and accountability, Distributor must:
- **Publish** the certificate subject (publisher identity) and either the certificate **SHA-256 fingerprint** or the public certificate chain used to sign the DLL (e.g., in the Launcher’s “About” page, documentation, or release notes).
- **Publish** the DLL’s **SHA-256 hash** for each distributed release of the signed DLL.
- **Provide** (upon Licensor’s reasonable request) the exact signed DLL release artifact corresponding to a published hash, for the purpose of verifying compliance with this Addendum.

## 6. Prohibited Content (“Skinhacking”)

Distributor must ensure the Launcher blocks installation/injection of all Prohibited Content below.

### 6.1 Competitive Integrity (Gameplay Advantage)

Content that provides unfair advantages in gameplay is strictly prohibited, including but not limited to:
- Visual indicators of ability ranges
- Hitbox visualization
- Any modifications that provide competitive advantages not available in the base game
- Modifications that alter game clarity to provide an unfair advantage

### 6.2 Paid Content Replication (IP / Cosmetic Piracy)

Content is prohibited where the primary intent is to replicate currently or previously available paid League of Legends content. This includes, without limitation:
- Direct replication of PBE or live manifest content corresponding to purchasable or previously purchasable League of Legends skins or cosmetic assets
- Content designed to be immediately recognizable as a paid League of Legends skin with minimal transformation
- Ports from other Riot Games IP or past updates that functionally replicate paid content without substantial original transformation or meaningful modification of equivalent live content where applicable

### 6.3 Permitted Derivative Works (Cosmetic / Creative Use)

Creative use may be permitted when all of the following are true:
- The content is **substantially transformed**
- The work is **original in nature**
- The intent is **not** to replicate purchasable content
- Proper attribution is provided where appropriate

## 7. Enforcement Obligations (Launcher-Level)

7.1 **Mandatory Blocking.** Distributor must implement and maintain enforcement at the Launcher level such that Prohibited Content (Section 6) is **blocked from being installed and/or injected** through the DLL.

7.2 **No Penalty Requirement.** Licensor does not require Distributor to impose user penalties. However, Distributor must ensure the practical outcome: **violating content is blocked from installation/injection.**

7.3 **Reasonable Maintenance.** Distributor must make reasonable efforts to keep blocking mechanisms effective against known prohibited content patterns relevant to its distribution ecosystem.

## 8. Restrictions on Modification, Reversing, and Tampering

8.1 **No Modification of the DLL.** Distributor must not modify, reverse engineer, patch, tamper with, or create Derivative Binaries of the DLL.

8.2 **No Circumvention.** Distributor must not bypass or disable technical controls intended to prevent prohibited content installation/injection.

## 9. Termination

9.1 **Automatic Termination.** Any violation of this Addendum immediately terminates the permission granted in Section 4.

9.2 **Effect of Termination.** Upon termination, Distributor must cease Distribution and use of the DLL and must remove hosted copies under its control.

## 10. Enforcement, Notices, and Reporting

10.1 **DMCA and Other Remedies.** Licensor may pursue remedies for violations, including submitting DMCA takedown notices or other legal claims where applicable.

10.2 **Reporting to Riot.** Licensor may report violations to Riot Games and/or cooperate with Riot regarding enforcement actions, including cease-and-desist processes.

## 11. No Endorsement; Trademarks

Distribution or use of the DLL does not imply endorsement by Licensor or Riot Games. No trademark rights are granted.

## 12. Disclaimer; Limitation of Liability

THE DLL IS PROVIDED “AS IS,” WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. TO THE MAXIMUM EXTENT PERMITTED BY LAW, LICENSOR SHALL NOT BE LIABLE FOR ANY INDIRECT, INCIDENTAL, SPECIAL, CONSEQUENTIAL, OR PUNITIVE DAMAGES ARISING FROM OR RELATED TO THE DLL OR THIS ADDENDUM.

## 13. Contact

Compliance inquiries and notices may be sent to: 0xcrauzer@proton.me
