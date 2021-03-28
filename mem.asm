;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; tiles page
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SECTION PAGE_2

PUBLIC _tileBuf
_tileBuf: defs 4096,0

PUBLIC _spriteTiles
_spriteTiles: defs 4096,0

; No more space

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; object page
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SECTION PAGE_3

; Fix size when updating the Object struct
PUBLIC _objects
_objects: defs 200*17, 0

PUBLIC _objectOwnerTable
_objectOwnerTable: defs 775, 0

PUBLIC _objectStateTable
_objectStateTable: defs 775, 0

PUBLIC _invObjects
_invObjects: defs 80*7, 0

PUBLIC _inventoryOffset
_inventoryOffset: defs 1, 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; page structure for scipts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SECTION PAGE_32 ;; to page 47

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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; two costume pages
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SECTION PAGE_48

PUBLIC _costume31
_costume31: defs 8192, 0

SECTION PAGE_49

_costume31_tail: defs (11234-8192), 0

PUBLIC _costume32
_costume32: defs 140, 0

PUBLIC _actCostumes
_actCostumes: defs 2048, 0

PUBLIC _costumesPtr
_costumesPtr: defs 80 * 2, 0

PUBLIC _actCostumesUsed
_actCostumesUsed: defw 0

PUBLIC _anchors
_anchors: defs 6, 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; graphics page
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SECTION PAGE_50

PUBLIC _nametable
_nametable: defs 1024, 0
PUBLIC _attributes
_attributes: defs 64, 0
PUBLIC _translationTable
_translationTable: defs 256, 0
