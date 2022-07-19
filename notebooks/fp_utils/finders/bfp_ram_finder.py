import pandas as pd
import numpy as np
from typing import Iterable
from fp_utils.finders.ram_finder import RamFinder


class BFPRamFinder(RamFinder):
    def __init__(self, df: pd.DataFrame):
        self.df = df

    def _find(self, fingerprint: pd.Series) -> Iterable[str]:
        d = self.df.parallel_apply(lambda row: np.all(fingerprint.values <= row.values), axis=1)
        return d[d].index
