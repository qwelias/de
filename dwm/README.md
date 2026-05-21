based on [bakkeby/dwm-flexipatch](github.com/bakkeby/dwm-flexipatch)

# changes

## known patches
- BAR_DWMBLOCKS_PATCH
- BAR_LTSYMBOL_PATCH
- BAR_STATUS_PATCH
- BAR_STATUSCMD_PATCH
- BAR_STATUSCOLORS_PATCH
- BAR_SYSTRAY_PATCH
- BAR_TAGS_PATCH
- BAR_UNDERLINETAGS_PATCH
- BAR_WINTITLE_PATCH
- CENTER_PATCH
- CENTER_TRANSIENT_WINDOWS_PATCH
- CFACTS_PATCH
- DRAGCFACT_PATCH
- DRAGMFACT_PATCH
- FLOATPOS_PATCH
- NOBORDER_PATCH
- PLACEMOUSE_PATCH
- RESIZEPOINT_PATCH
- RESTARTSIG_PATCH
- SEAMLESS_RESTART_PATCH
- SHIFTTAG_PATCH
- SHIFTVIEW_PATCH
- VANITYGAPS_PATCH
- GAPPLESSGRID_LAYOUT
- TILE_LAYOUT
- MONOCLE_LAYOUT

## other hacky changes
- expose overview mode based on GAPPLESSGRID_LAYOUT
    - handling keyrelease event Super_L
    - handling some keypresses based on active layout
    - handling some button presses based on active layout
    - changes to setlayout
    - pop client on focus