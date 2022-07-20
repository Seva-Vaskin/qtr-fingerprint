import pandas as pd
from typing import Iterable, Optional
import random

from fp_utils.catch_time import CatchTime
from fp_utils.logger import Logger
from fp_utils.finders.finder import Finder
from fp_utils.tests.speed_test_stat import SpeedTestStat
from fp_utils.consts import FingerprintType, FingerprintsArrayType
from fp_utils import settings


class FinderSpeedTester:
    def __init__(self, finders: Iterable[Finder]) -> None:
        self.finders = finders

    def test_one(self, fingerprint: FingerprintType, ans_count: Optional[int] = None,
                 verbose: bool = False) -> SpeedTestStat:
        fingerprint = settings.fingerprint_to_series(fingerprint)
        measurements = dict()
        for finder in self.finders:
            catch_time = CatchTime(finder.__class__.__name__)
            with catch_time:
                list(finder.find(fingerprint, ans_count))
            measurements[finder.__class__] = [catch_time]
            Logger.log(catch_time, verbose)
        return SpeedTestStat(measurements)

    def test_all(self, fingerprints: FingerprintsArrayType, ans_count: Optional[int] = None,
                 verbose: bool = False) -> SpeedTestStat:
        fingerprints = settings.fingerprint_array_to_iterable_of_series(fingerprints)
        stat = SpeedTestStat()
        Logger.log('-----', verbose)
        for i, fingerprint in enumerate(fingerprints):
            Logger.log(f'Test #{i:03d}', verbose)
            local_measurements = self.test_one(fingerprint, ans_count, verbose)
            stat += local_measurements
            Logger.log('-----', verbose)
        return stat

    def test_random(self, fingerprints: FingerprintsArrayType, test_count: int, ans_count: Optional[int] = None,
                    verbose: bool = False) -> SpeedTestStat:
        if isinstance(fingerprints, pd.DataFrame):
            return self.test_all(fingerprints.sample(test_count), ans_count, verbose)
        else:
            return self.test_all(random.sample(list(fingerprints), test_count), ans_count, verbose)
