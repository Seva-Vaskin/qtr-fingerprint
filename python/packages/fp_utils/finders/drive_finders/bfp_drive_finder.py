from abc import ABC

import pandas as pd
from typing import Iterable, Optional
from fp_utils.finders.drive_finders.drive_finder import DriveFinder
from fp_utils.consts import PathType
from fp_utils.settings import is_sub_fingerprint


class BFPDriveFinder(DriveFinder, ABC):
    def __init__(self, df: pd.DataFrame, directory: PathType, unique_id: Optional[str] = None, *args, **kwargs):
        self.packer.pack(df, self.index_file)

    def find_all(self, fingerprint: pd.Series) -> Iterable[str]:
        df = self.packer.unpack(self.index_file)
        d = df.parallel_apply(lambda row: is_sub_fingerprint(fingerprint.values, row), axis=1)
        return d[d].index
