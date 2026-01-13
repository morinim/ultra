# Security Policy

## Supported versions

**We only provide security patches for the latest stable version**. We recommend all users upgrade immediately when a new release is available.

Vulnerability priority is determined by the [CVSS v4.0 Rating][CVSS]:

| CVSS    | Patch priority | Target resolution |
| ---     | ---            | ---               |
| **9.0 - 10.0 (Critical)** | Immediate | As soon as technically possible |
| **4.0 - 8.9 (Medium/High)** | High | Targeted for the next point release |
| **0.1 - 3.9 (Low)** | Standard | Best effort; usually included in the next scheduled release |


## Reporting a Vulnerability

If you discover a security vulnerability, please **do not open a public issue**.

Instead, please use the email specified on our [contact page][contact].


### Our Process

1. **Acknowledgement**. You will receive a response within 3 business days.
2. **Investigation**. We will work with you to understand the scope and impact.
3. **Fix & Disclosure**. Once a fix is ready, we will coordinate a release and credit you for the discovery (if desired).

We ask that you practice responsible disclosure and refrain from sharing details publicly until a patch has been released.

---

## Verifying Releases

To ensure the integrity of our software, all important version tags (major releases, minor releases and security fixes) are cryptographically signed.

You can verify these signatures using our [public GPG key][key].

### Verification Steps

Import the key:

```Bash
gpg --import .github/KEYS
```

and ensure the imported key matches our official fingerprint: `9EB0 2E6E 5355 2D6E 6B20 C09A CA8B 2AE5 C5E3 D972`

To verify a tag:

```Bash
git tag -v v1.0.0
```

A "good signature" message confirms that the release was signed by the ULTRA project and has not been altered since it was tagged.


[contact]: https://ultraevolution.org/contact/
[CVSS]: https://www.first.org/cvss/calculator/4.0
[key]: https://github.com/morinim/ultra/blob/main/.github/KEYS
