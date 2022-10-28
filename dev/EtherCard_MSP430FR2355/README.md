This is a port of the Arduino EtherCard library to the MSP430FR2355.

As we only require minimal functionality (specifically only operation at the link layer), large amounts of the above original library have been fully deleted to make porting easier.

The original EtherCard library was GPLv2 licensed, making it compatible with this project's GPLv3 license.

For reference, the original project's README.md has been preserved in README~EtherCard.md.

The copy ported to the MSP430FR2355 originated from [their Github](https://github.com/njh/EtherCard/tree/main/src) at tag 1.1.0 (commit 9025b6d)