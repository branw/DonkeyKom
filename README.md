# DonkeyKom

A toolkit for achieving [direct kernel object manipulation][dkom] (DKOM) on
modern Windows systems using certified yet vulnerable drivers, including:

1. ASUS memory mapping driver (`asmmap.sys`/`asmmap64.sys`) ([\[a\]][1a]
[\[b\]][1b] [\[c\]][1c])

[1a]: https://codeinsecurity.wordpress.com/2016/06/12/asus-uefi-update-driver-physical-memory-readwrite/
[1b]: https://www.exploit-db.com/exploits/39785/
[1c]: https://github.com/waryas/EUPMAccess

[dkom]: https://en.wikipedia.org/wiki/Direct_kernel_object_manipulation

## Usage

### Building

By the platform-dependent scope of this project, only support for building on
Windows is provided.

### Command-line Interface

A command-line interface is provided through the `CLI` project to demonstrate
usage of the toolkit. 

## License

DonkeyKom is released under the MIT License. See `LICENSE.txt` for more
information.
