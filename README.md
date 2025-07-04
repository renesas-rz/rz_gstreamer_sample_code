# RZ GStreamer Sample Code

This is a GStreamer Sample Code that provided for the RZ Family MPUs from Renesas Electronics.

## Specific Boards and MPUs Sample Code

To get Sample Code for your boards and MPUs, please access specific link below:

- Linux kernel version 6.1 and Yocto version 5.0 (Scrathgap):
    1. [RZ/G2L Group](../vlp-4.0.x_rz-g2l)
        * Board: RZ/G2L SMARC Evaluation Kit / MPU: R9A07G044L (RZ/G2L)

- Linux kernel version 5.10 and Yocto version 3.1 (Dunfell):
    1. [RZ/G2L Group and RZ/V2L Group](../vlp-3.0.x_rz-g2l_rz-v2l)
        * Board: RZ/G2L SMARC Evaluation Kit / MPU: R9A07G044L (RZ/G2L)
        * Board: RZ/V2L SMARC Evaluation Kit / MPU: R9A07G054L (RZ/V2L)
    2. [RZ/V2N Group](../ai-sdk-5.xx_rz-v2n)
        * Board: RZ/V2N Evaluation Board Kit / MPU: R9A09G056N44 (RZ/V2N)
    3. [RZ/V2H Group](../ai-sdk-5.xx_rz-v2h)
        * Board: RZ/V2H Evaluation Board Kit / MPU: R9A09G057H4 (RZ/V2H)
## Layout

This GStreamer Sample Code provided as:

```
<this project>
 |- <combination of VLP/AI-SDK version with boards and MPUs group target branch>
     |- <sample code for each use-case folder>
```

Documentation for each use-case exist on each folder.

## Release

Sample Code can be access either from branch or tag.

Branches are prepared for combination of VLP/AI-SDK version with boards and MPUs group.

Tag name is combination of VLP/AI-SDK version and specific MPU (of EVK).

See all releases [here](https://github.com/renesas-rz/rz_gstreamer_sample_code/tags).
