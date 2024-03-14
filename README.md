<!DOCTYPE html>
<html lang="en">
    <body>
        <h1>WHD Archive Extractor</h1>
        <p>This Amiga CLI program simplifies the extraction of large numbers of LHA archives commonly used in WHDLoad. Designed with WHDLoad Download Tool users in mind, it automates the process of searching for archives in all subfolders and extracting them to a specified target path while preserving the original folder structure.</p> 
        <h2>Key Features</h2>
        <ul>
            <li>Scanning input folders and subfolders for LHA and LZX archives</li>
            <li>Extracting the archives using the LHA/LZX program to an output folder</li>
            <li>Preserving the subfolder structure from the input folder during extraction</li>
            <li>Extracting only new or updated files to avoid unnecessary duplication</li>
        </ul>
            <h2>Prerequisites</h2>
            To use this program, ensure the following software is installed in the C: directory<br/>
            <ul>
                <li>LHA: can be downloaded from <a href="https://aminet.net/package/util/arc/lha">aminet.net/package/util/arc/lha</a>.</li>
                <li>LZX: required for a small number of archives, and this can be downloaded from <a href="https://aminet.net/package/util/arc/lzx121r1">aminet.net/package/util/arc/lzx121r1</a>.</li>
            </ul>
            <h2>Usage</h2>
        <pre><code>$ WHDArchiveExtractor &lt;source_directory&gt; &lt;output_directory&gt;</code></pre>
        <p>For example:</p>
        <pre><code>$ WHDArchiveExtractor PC0:WHDLoad/Beta DH0:WHDLoad/Beta</code></pre>
        <p>This will scan the PC0:WHDLoad/Beta directory and extract all LHA archives found to the DH0:WHDLoad/Beta directory, preserving the folder structure.</p>
            <h2>Building</h2>
        <p>To compile the program, use an Amiga C compiler, such as SAS/C, with the provided source code.</p>
            <h2>Disclaimer</h2>
        <p>Before using this program, please make sure to backup any important data or files. The creators and contributors of this software are not responsible for any data loss or damage that may occur as a result of using this program.</p>
            <h2>License</h2>
        <p>This project is licensed under the <a href="https://opensource.org/licenses/MIT">MIT License</a>.</p>
    </body>
</html>
