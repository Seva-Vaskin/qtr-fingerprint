import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from typing import List, Dict

from substructure_finder.search_engines.bucket_search_engine import BucketSearchEngine
from substructure_finder.molecule import Molecule
from substructure_finder import utils
from substructure_finder.db_filesystem import DbFilesystem


class BucketsInitializer:
    def __init__(self, db_filesystem: DbFilesystem, raw_db_name: str, db_name: str, columns_count: int):
        self.db_filesystem = db_filesystem
        self.raw_db_name = raw_db_name
        self.db_name = db_name
        self.columns_count = columns_count
        self.raw_bucket_paths = self.db_filesystem.bucket_dir_paths(raw_db_name)

    def init_buckets(self) -> None:
        with ProcessPoolExecutor() as executor:
            tasks = []
            for _, buckets in self.db_filesystem.base_dir_to_buckets(self.raw_db_name).items():
                tasks.append(executor.submit(self._init_buckets, buckets))
            for task in tasks:
                task.result()

    def _init_buckets(self, buckets: List[int]) -> None:
        for bucket in buckets:
            self._init_bucket(self.raw_bucket_paths[bucket], bucket)

    def _init_bucket_path(self, bucket: int) -> Path:
        raw_bucket_path = self.raw_bucket_paths[bucket]
        raw_bucket_base_path = self.db_filesystem.bucket_base_dir(raw_bucket_path)
        bucket_path = raw_bucket_base_path / self.db_name / str(bucket) / 'data.pickle'
        bucket_path.parent.mkdir(parents=True, exist_ok=True)
        return bucket_path

    def _init_bucket(self, raw_bucket_path: Path, bucket: int) -> None:
        print(f"Start init {bucket}\n", end='', file=sys.stderr)
        with self.db_filesystem.rb_file_from_bucket_dir(raw_bucket_path).open('rb') as stream:
            molecules = Molecule.load_molecules_from_rb_stream(stream)
        columns_file = self.db_filesystem.col_file_from_bucket_dir(raw_bucket_path)
        columns = utils.load_columns_from_file(columns_file)[:self.columns_count]
        assert len(columns) == self.columns_count
        bucket_file_path = self._init_bucket_path(bucket)
        bucket_search_engine = BucketSearchEngine(molecules, columns)
        bucket_search_engine.dump(bucket_file_path)
        print(f"Finish init {bucket}\n", end='', file=sys.stderr)
