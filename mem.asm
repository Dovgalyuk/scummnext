SECTION PAGE_2

PUBLIC _tileBuf
_tileBuf: defs 4096,0

PUBLIC _spriteTiles
_spriteTiles: defs 4096,0

; No more space

SECTION PAGE_3

PUBLIC _objectOwnerTable
_objectOwnerTable: defs 775, 0

PUBLIC _objectStateTable
_objectStateTable: defs 775, 0

; page structure for scipts
SECTION PAGE_32

PUBLIC _scriptBytes
_scriptBytes: defs 4096, 0
PUBLIC _script_id
_script_id: defs 2, 0
PUBLIC _script_room
_script_room: defs 1, 0
PUBLIC _script_roomoffs
_script_roomoffs: defs 2, 0
PUBLIC _script_offset
_script_offset: defs 2, 0
PUBLIC _script_delay
_script_delay: defs 4, 0
PUBLIC _script_resultVarNumber
_script_resultVarNumber: defs 2, 0

; No more space
