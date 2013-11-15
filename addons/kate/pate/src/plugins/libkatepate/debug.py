# -*- coding: utf-8 -*-

"""Debug utilities useful in Qt/KDE/Kate context
"""

def nsmap(ns, enum):
    """Maps enum constants to their names.

    Useful for debugging C++ Enums:

        >>> role_e = nsmap(Qt, Qt.ItemDataRole)
        >>> role_e[Qt.DisplayRole]
        'DisplayRole'
        >>> len(role_e)
        16
    """
    return {num: name for name, num in vars(ns).items() if isinstance(num, enum)}
