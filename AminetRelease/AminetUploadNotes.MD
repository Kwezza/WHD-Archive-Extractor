The Readme file
---------------

img { max-width: 50%; }

### Introduction

Each readme file starts with a few lines that give a short, machine-readable overview over various attributes of the upload. These lines, which are also referred to as "fields", are followed by an empty line. Below that, you can print a short introduction to your upload, include your copyright notice or simply add whatever documentation is inside the archive. A typical Aminet readme file would look like this:

      Short:        Shows JPEG & IFF pics. AGA support
      Uploader:     aminet@aminet.net (Urban Mueller)
      Author:       aminet@aminet.net (Urban Mueller)
      Type:         gfx/show
      Version:      1.2
      Architecture: m68k-amigaos >= 2.0.4
      Distribution: Aminet
      Kurz:         Zeigt JPEG und IFF an, AGA-Support
    
      This all new picture displayer is capable of showing almost any image
      type in any resolution. Main features include:
      - Scrolling screens
      - CLI and WB interface
       ......
    

This example demonstrates the fields that can be used in your readme file, a detailed explanation for each of them is following below. Some of these fields are are absolutely mandatory (i.e. you risk having your upload deleted if you do not specify them), some of them are recommended and some of them are completely optional.

### Short:

This field needs to be the first line of the readme file. It lets you specify a short description of your upload which will be displayed in Aminet's directory tree and in our daily and weekly e-mail newsletters. This description must not be longer than 40 characters. Don't repeat the file name here and do not include any version numbers or target platforms in the Short: field - try to explain what the program **does**. Music should specify the style and author. Don't boast or use much uppercase.

If your upload requires a language other than English, please mention that language in the Short: description.

### Author:

This is the place where you can indicate who created the piece of software you are uploading. You can optionally include an email here, but keep in mind that it will not be obfuscated, e.g. it might be harvested by spam bots.

### Uploader:

Lets you indicate your email address so we can contact you if something goes wrong (and this happens more often than you think). You have to provide a clearly readable, non-obfuscated e-mail address here. Don't worry about recieving lots of SPAM mails; Your e-mail address will be stripped for "@" and "." characters in the readme file before it gets added to the Aminet archive. This way it remains human readable, but useless for spam bots.

### Type:

Here you need to specify a target directory for your upload. Check the [index of Aminet directories](http://www.aminet.net/info/adt/tree) for a list of possible values.

### Architecture:

This field is used to specify the target architecture of your upload. Here is a list of possible target architectures:

*   m68k-amigaos
*   ppc-amigaos
*   ppc-morphos
*   ppc-powerup
*   ppc-warpup
*   i386-aros
*   i386-amithlon
*   generic

If your package does not contain **any** binaries, you should use "generic". If your package contains binaries for one or more platforms, you need to specify the appropriate architecture(s) here. You can list several architectures, separated by semicolons. Examples:

Package contains binaries for classic AmigaOS:

`Architecture: m68k-amigaos`

Package contains binaries for MorphOS:

`Architecture: ppc-morphos`

Package contains binaries for AmigaOS 4 and WarpUp:

`Architecture: ppc-amigaos; ppc-warpup`

Note: you only specify architectures that the binaries in your package were **compiled** for. You do not specify architectures that your binaries happen to be compatible with. If you are uploading a binary for m68k-amigaos, that's the only architecture you should specify - no matter if your binary also happens to run on other platforms (AmigaOS 4, MorphOS) by means of emulation.

Additionally, a version string together with a modifier (>=, >, =, \\<, \\<=) can be added to each architecture you specify, to point out the minimum or maximum version number of said architecture required to run your program. Examples:

Package contains binaries for MorphOS 1.4 or higher:

`Architecture: ppc-morphos >= 1.4.0`

Package contains binaries for AmigaOS 3.9 and AmigaOS 4.0 or higher:

`Architecture: m68k-amigaos >= 3.9.0; ppc-amigaos >= 4.0.0`

Package contains binaries for AmigaOS 1.3 or lower:

`Architecture: m68k-amigaos <= 1.3.3`

To sum it up, the template for the architecture: field looks like this:

`Architecture: ARCH1 [MODIFIER VERSION] [; ARCH2 [MODIFIER VERSION] ... ]`

Final note: Although it is allowed, we do not encourage putting several binaries for different architectures into one and the same archive - if possible, create a separate archive for each architecture. A good example would be [Frank Wille's Quake port](http://aminet.net/search?path=game/shoot&desc=quake1+port).

### Replaces:

Here you can specify files that should be replaced by your upload. Give full path, Unix wildcards are allowed in the filename. Examples:

`Replaces: biz/patch/PageStreamPatch1.lha`

`Replaces: biz/patch/PageStreamPatch*`

You can specify several files here, separated by semicolons. You do not need to specify this field if you are going to overwrite an existing package which has the same name as your current upload.

### Requires:

Other Aminet archives that your upload needs to work, with full path. Example:

`Requires: util/libs/mui38usr.lha`

Also name memory and chipset requirements here.

### Version:

The version number of your upload. Don't use version numbers in file names if possible.

### Distribution:

There are two legal values for this field, "NoCD" and "Aminet" - do not specify anything else here. With "Distribution: NoCD", you make sure that your upload will not be distributed on the CDs or DVDs created by the Aminet team. With "Distribution: Aminet" you restrict distribution of your upload to Aminet (including Aminet CDs and other media), i.e. you stop other groups from redistributing it.

### Kurz:

This is the German version of Short: (see above).




ReadMe file requirements
------------------------

### Introduction

Each readme file starts with a few lines that give a short, machine-readable overview over various attributes of the upload. These lines, which are also referred to as "fields", are followed by an empty line. Below that, you can print a short introduction to your upload, include your copyright notice or simply add whatever documentation is inside the archive. A typical Aminet readme file would look like this:

      Short:        Shows JPEG & IFF pics. AGA support
      Uploader:     aminet@aminet.net (Urban Mueller)
      Author:       aminet@aminet.net (Urban Mueller)
      Type:         gfx/show
      Version:      1.2
      Architecture: m68k-amigaos >= 2.0.4
      Distribution: Aminet
      Kurz:         Zeigt JPEG und IFF an, AGA-Support
    
      This all new picture displayer is capable of showing almost any image
      type in any resolution. Main features include:
      - Scrolling screens
      - CLI and WB interface
       ......
    

This example demonstrates the fields that can be used in your readme file, a detailed explanation for each of them is following below. Some of these fields are are absolutely mandatory (i.e. you risk having your upload deleted if you do not specify them), some of them are recommended and some of them are completely optional.

### Short:

This field needs to be the first line of the readme file. It lets you specify a short description of your upload which will be displayed in Aminet's directory tree and in our daily and weekly e-mail newsletters. This description must not be longer than 40 characters. Don't repeat the file name here and do not include any version numbers or target platforms in the Short: field - try to explain what the program **does**. Music should specify the style and author. Don't boast or use much uppercase.

If your upload requires a language other than English, please mention that language in the Short: description.

### Author:

This is the place where you can indicate who created the piece of software you are uploading. You can optionally include an email here, but keep in mind that it will not be obfuscated, e.g. it might be harvested by spam bots.

### Uploader:

Lets you indicate your email address so we can contact you if something goes wrong (and this happens more often than you think). You have to provide a clearly readable, non-obfuscated e-mail address here. Don't worry about recieving lots of SPAM mails; Your e-mail address will be stripped for "@" and "." characters in the readme file before it gets added to the Aminet archive. This way it remains human readable, but useless for spam bots.

### Type:

Here you need to specify a target directory for your upload. Check the [index of Aminet directories](http://www.aminet.net/info/adt/tree) for a list of possible values.

### Architecture:

This field is used to specify the target architecture of your upload. Here is a list of possible target architectures:

*   m68k-amigaos
*   ppc-amigaos
*   ppc-morphos
*   ppc-powerup
*   ppc-warpup
*   i386-aros
*   i386-amithlon
*   generic

If your package does not contain **any** binaries, you should use "generic". If your package contains binaries for one or more platforms, you need to specify the appropriate architecture(s) here. You can list several architectures, separated by semicolons. Examples:

Package contains binaries for classic AmigaOS:

`Architecture: m68k-amigaos`

Package contains binaries for MorphOS:

`Architecture: ppc-morphos`

Package contains binaries for AmigaOS 4 and WarpUp:

`Architecture: ppc-amigaos; ppc-warpup`

Note: you only specify architectures that the binaries in your package were **compiled** for. You do not specify architectures that your binaries happen to be compatible with. If you are uploading a binary for m68k-amigaos, that's the only architecture you should specify - no matter if your binary also happens to run on other platforms (AmigaOS 4, MorphOS) by means of emulation.

Additionally, a version string together with a modifier (>=, >, =, \\<, \\<=) can be added to each architecture you specify, to point out the minimum or maximum version number of said architecture required to run your program. Examples:

Package contains binaries for MorphOS 1.4 or higher:

`Architecture: ppc-morphos >= 1.4.0`

Package contains binaries for AmigaOS 3.9 and AmigaOS 4.0 or higher:

`Architecture: m68k-amigaos >= 3.9.0; ppc-amigaos >= 4.0.0`

Package contains binaries for AmigaOS 1.3 or lower:

`Architecture: m68k-amigaos <= 1.3.3`

To sum it up, the template for the architecture: field looks like this:

`Architecture: ARCH1 [MODIFIER VERSION] [; ARCH2 [MODIFIER VERSION] ... ]`

Final note: Although it is allowed, we do not encourage putting several binaries for different architectures into one and the same archive - if possible, create a separate archive for each architecture. A good example would be [Frank Wille's Quake port](http://aminet.net/search?path=game/shoot&desc=quake1+port).

### Replaces:

Here you can specify files that should be replaced by your upload. Give full path, Unix wildcards are allowed in the filename. Examples:

`Replaces: biz/patch/PageStreamPatch1.lha`

`Replaces: biz/patch/PageStreamPatch*`

You can specify several files here, separated by semicolons. You do not need to specify this field if you are going to overwrite an existing package which has the same name as your current upload.

### Requires:

Other Aminet archives that your upload needs to work, with full path. Example:

`Requires: util/libs/mui38usr.lha`

Also name memory and chipset requirements here.

### Version:

The version number of your upload. Don't use version numbers in file names if possible.

### Distribution:

There are two legal values for this field, "NoCD" and "Aminet" - do not specify anything else here. With "Distribution: NoCD", you make sure that your upload will not be distributed on the CDs or DVDs created by the Aminet team. With "Distribution: Aminet" you restrict distribution of your upload to Aminet (including Aminet CDs and other media), i.e. you stop other groups from redistributing it.

### Kurz:

This is the German version of Short: (see above).


Uploading instructions
What to upload
This site is intended for the distribution of any type of freely distributable software. However, due to copyright laws and internet provider restrictions, we have to set the following conditions:

DO NOT UPLOAD:

Unlicensed copies of commercial software
Software with a license in conflict with Aminet's nature
Samples from copyrighted CDs, TV or radio broadcasts
Musical mods that are cover versions from copyrighted songs
Images or animations from copyrighted image sources
Images depicting persons who didn't agree with the distribution
Images of sexual content
GNU software without sources or written offer
DMS files (use ADF if you really need to upload a disk image)
Insulting/obscene material
Please respect these conditions, or you could bring Aminet in danger of being shut down. Aminet staff reserves the right to remove any contribution without warning if it doesn't comply to the above rules or is otherwise unfit.

The readme file
Each upload to Aminet needs to be accompanied by a readme file: Along with PictureView.lha you would upload PictureView.readme, along with w2c-sources.tar.gz you would upload w2c-sources.readme. The format of this readme file needs to follow certain rules which are explained in The Readme file.

Where to upload
You need to use an FTP client to upload to Aminet. Connect to ftp://main.aminet.net/new using anonymous as user and your email address as password.

Filename conventions, accepted filetypes
The maximum file name length is 30 characters including the filename suffix (.lha, .adf, .readme). Mixed case is OK, but it should be mainly lowercase. If your file name is generic (ls, pipe), append your initials (pipe-JU). Filenames should only contain letters, digits, dots, underscores ("_") and hyphens ("-").

The following file formats are accepted on Aminet:

file type	notes	filename extensions
Archives	LhA, self extracting LhA or ZIP	.lha, .run, .zip
Disk images	ADF images or gzipped ADF images	.adf, .adz
Pictures	JPEG, PNG or GIF pictures	.jpg, .png, .gif
Documents	PDF documents or plain text	.pdf, .txt
Audio recordings	Ogg Theora, MP3	.ogg, .mp3
Videos	MPEG	.mpg
Compressors	For packing ISOs or Tarballs (see below)	.gz, .bz2
Tarballs	Optionally compress with one of the above	.tar, .tar.gz, .tgz, .tar.bz2
 
Note that the Aminet team recommends the use of file formats that are not affected by patent, copyright or trademark issues and for which open source implementations are available. MP3, MPEG and GIF are only supported for historical reasons.

Further notes for uploaders
Some other things you should keep in mind when preparing an Aminet upload:

If your upload is shareware, restricted or just a demo version, please mention that in the readme.
Make sure your readme file is formatted correctly: Lines should not be longer than 78 characters, a line should be terminated only by a linefeed (LF) character (that's the way AmigaOS and Unix do it), *not* with both CR (carriage return) and LF (that's the Windows way).
Version numbers should be part of the readme file, not part of the filename.
You can overwrite old versions of your uploads by uploading again using the same file name. This is the preferred way to do updates.
Don't update too often - one update per week should be sufficient. Whenever you update an existing archive, make sure you specify a different version number and clearly mark the changes.
Check list
Did you specify all required fields (Short:, Uploader:, Type: and Architecture:) in the readme file?
Are there older versions of your package already available on Aminet? If yes, make sure you use the replaces: field to remove them from Aminet.
Are the archives okay? Use UNIX LhA to find out.
Is there a .readme file for *every* file you are about to upload?
Are the base names of the .readme and the archive the same? Same case?
Are all filenames 30 characters or shorter?
Does none of the archives contain copyrighted or Workbench material?
Thanks for checking, you're saving us a lot of work.