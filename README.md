# Arconda

<div align="center">
  <p><strong>Advanced PNG-Based File Steganography Tool</strong></p>
</div>

## Overview

Arconda is a sophisticated steganography tool that allows you to hide files within PNG images using AES-256-GCM encryption and custom PNG chunk embedding. It provides a secure way to conceal and transport confidential data within seemingly innocent image files, combining military-grade encryption with steganographic techniques.

## Features

- **AES-256-GCM Encryption** - Industry-standard authenticated encryption
- **PBKDF2 Key Derivation** - 200,000 iterations for password strengthening
- **Custom PNG Chunk Embedding** - Uses "rNDm" chunk type for hidden data
- **Automatic Compression** - zlib compression before encryption
- **Secure Password Input** - Hidden terminal input with confirmation
- **File Integrity Verification** - GCM authentication tags prevent tampering
- **Directory Output Support** - Extract files to specified directories

## Prerequisites

- OpenSSL (1.1 or higher)
- zlib development libraries
- GCC compiler
- Make

## 📦 Installation

```bash
sudo apt update

# Install GCC and Make
sudo apt install -y build-essential

# Install OpenSSL (1.1+ usually comes by default in modern systems)
sudo apt install -y openssl libssl-dev

# Install zlib development libraries
sudo apt install -y zlib1g-dev

# Clone the repository
git clone https://github.com/kUrOSH1R0oo/arconda.git
cd arconda

# Build the project
make

# Install system-wide (optional)
sudo make install
```

## Usage

### Encode (Hide a File)

```bash
# Basic encoding
arconda -e -i cover.png -o output.png -s secret.txt

# With custom output directory
arconda -e -i cover.png -o ./stego/output.png -s secret.pdf
```

### Decode (Extract Hidden File)

```bash
# Extract to current directory
arconda -d -i stego.png

# Extract to specific directory
arconda -d -i stego.png -o ./extracted_files

```

## Command Line Options

| Option | Description |
|--------|------------|
| `-e`, `--encode` | Encode mode (hide file inside PNG) |
| `-d`, `--decode` | Decode mode (extract hidden file) |
| `-i`, `--input FILE` | Input PNG file |
| `-o`, `--output FILE/DIR` | Output PNG (encode mode) or output directory (decode mode) |
| `-s`, `--secret FILE` | File to hide (encode mode only) |
| `-h`, `--help` | Show help message |


## Project Structure

```text
arconda/
├── arconda.c           # Main CLI interface
├── core/
│   ├── crypto.c        # Encryption/decryption logic
│   ├── crypto.h        
│   ├── filepack.c      # File packing/unpacking
│   ├── filepack.h      
│   ├── png.c           # PNG chunk manipulation
│   ├── png.h           
│   ├── util.c          # Utility functions
│   └── util.h          
├── Makefile            
└── README.md
```

## Technical Explanation: PNG & Steganography

### What is PNG?

PNG (Portable Network Graphics) is an image format designed for lossless compression. Unlike JPEG, PNG preserves every pixel exactly as intended, making it ideal for graphics, logos, and screenshots. But what makes PNG special for steganography is its chunk-based structure.

A PNG file is built like LEGO bricks - it consists of multiple chunks stacked together:

```text
┌─────────────────┐
│   PNG Signature │ (8 bytes - identifies as PNG)
├─────────────────┤
│   IHDR Chunk    │ (Image header - dimensions, color type)
├─────────────────┤
│   IDAT Chunk    │ (Image data - compressed pixel data)
├─────────────────┤
│   IDAT Chunk    │ (More image data)
├─────────────────┤
│   ...           │
├─────────────────┤
│   IEND Chunk    │ (End of image marker)
└─────────────────┘
```

Each chunk follows this structure:

- **Length (4 bytes):** How big is this chunk's data?
- **Type (4 bytes):** What kind of chunk is this? (IHDR, IDAT, IEND, etc.)
- **Data (variable):** The actual chunk content
- **CRC (4 bytes):** Checksum for integrity

## How Arconda Hides Files

Here's the genius part: **PNG viewers ignore chunks they don't recognize.** If a PNG parser encounters a chunk type it doesn't understand (like "rNDm"), it simply skips it and moves to the next chunk. The image still displays perfectly because all the standard chunks (IHDR, IDAT, IEND) remain untouched.

Arconda exploits this behavior by:

- **Reading the original PNG:** We load the carrier image
- **Processing your secret file:** Compress and encrypt it
- **Injecting a custom chunk:** We add our own chunk type "rNDm" right before the IEND chunk
- **The magic:** Any PNG viewer sees "rNDm", doesn't recognize it, and ignores it completely

```text
Original PNG structure:
┌─────┬─────┬─────┬─────┐
│IHDR │IDAT │IDAT │IEND │
└─────┴─────┴─────┴─────┘

After Arconda:
┌─────┬─────┬─────┬─────┬─────┐
│IHDR │IDAT │IDAT │rNDm │IEND │ ← Hidden data chunk
└─────┴─────┴─────┴─────┴─────┘
                         ↑
                Your secret file lives here!
```

## But How Can It Hide Large Files?

This is where it gets interesting. PNG chunks have a **4-byte length field**, meaning each chunk can hold up to **4.29 GB** of data (2³²-1 bytes). In theory, you could hide a 4GB file in a single chunk!

However, practical limitations exist:

1. **File size overhead:** The encrypted+compressed data adds about 0-50% overhead depending on file type
  - **Text files:** Compress very well (90%+ reduction)
  - **Already compressed files (ZIP, JPEG, MP3):** Minimal compression gain
  - **Encrypted data:** Doesn't compress at all

2. **PNG viewer compatibility:** Some strict PNG validators might complain about extremely large custom chunks

3. **Memory constraints:** The tool must load the entire file into memory

### Real-world capacity example:

A 10MB PNG can typically hide:
  - 15-20MB of highly compressible text
  - 5-8MB of already compressed data
  - 3-5MB of encrypted data

## The Complete Process

When you hide a file, Arconda performs this transformation:

```text
┌──────────────┐
│  secret.pdf  │  (50 MB)
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   pack_file  │ ← Adds filename and size metadata
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   compress   │ ← zlib compression (now 35 MB)
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   encrypt    │ ← AES-256-GCM (still 35 MB + overhead)
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  embed in    │ ← Injected as "rNDm" chunk
│   PNG file   │    before IEND
└──────────────┘
```

*The final PNG still opens normally in any image viewer, but contains your secret data hidden in plain sight!*

## Why This Works

PNG's design philosophy of **ignore unknown chunks** was intended for forward compatibility - new features could be added without breaking old viewers. Arconda simply repurposes this feature for steganography.

The hidden data is:
  - Invisible to casual inspection
  - Undetectable by standard image analysis
  - Secure behind AES-256 encryption
  - Portable across any system that supports PNG

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Author

Ivan Dizon (Kur0Sh1r0)

GitHub: https://github.com/kUrOSH1R0oo

Protonmail: kur0sh1r0@protonmail.com








