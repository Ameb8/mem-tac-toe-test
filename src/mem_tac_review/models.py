"""Data models for MemTacToe review harness."""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Optional, Union


class PlayerMark(str, Enum):
    """Player marks and empty cell marker."""

    X = "X"
    O = "O"
    EMPTY = "."

    @classmethod
    def opponent(cls, mark: str) -> str:
        if mark == cls.X:
            return cls.O.value
        if mark == cls.O:
            return cls.X.value
        raise ValueError(f"Unknown player mark: {mark}")


class GameOutcome(str, Enum):
    """Reference game outcome."""

    IN_PROGRESS = "IN_PROGRESS"
    X_WINS = "X_WINS"
    O_WINS = "O_WINS"
    DRAW = "DRAW"


class SessionStatus(str, Enum):
    """Scenario execution status."""

    COMPLETED = "completed"
    BUILD_ERROR = "build error"
    LAUNCH_ERROR = "launch error"
    TIMED_OUT = "timed out"
    INTERRUPTED = "interrupted"
    NOT_RUN = "not run"


@dataclass(frozen=True)
class Coordinate:
    """Zero-based board coordinate."""

    row: int
    col: int

    def __str__(self) -> str:
        return f"({self.row}, {self.col})"


class ActionType(str, Enum):
    """Type of action in a scenario."""

    MOVE = "move"
    RAW_INPUT = "raw_input"
    WAIT = "wait"
    CLOSE_INPUT = "close_input"


@dataclass
class Action:
    """Single interaction action in a scenario."""

    type: ActionType = ActionType.MOVE
    player: str = "X"  # "X", "O", "p1", "p2"
    coord: Optional[Coordinate] = None
    raw_text: Optional[str] = None
    delay_ms: int = 0
    description: str = ""
    is_intentional_invalid: bool = False
    expect_checkpoint: bool = True


@dataclass
class Scenario:
    """A test scenario describing actions and expected game flow."""

    id: str
    name: str
    description: str
    grid_size: int = 3
    starting_player: str = "X"
    actions: list[Action] = field(default_factory=list)
    tags: list[str] = field(default_factory=list)
    is_cli_scenario: bool = False
    cli_args_override: Optional[list[str]] = None
    expected_cli_behavior: str = ""


@dataclass
class Profile:
    """Submission profile configuring build, launch, and pacing."""

    name: str
    build_cmd: Optional[str] = None
    work_dir: str = "."
    topology: str = "dual"  # "dual" (two separate processes) or "single" (shared terminal)
    launch_p1: list[str] = field(default_factory=lambda: ["./player1", "{N}"])
    launch_p2: list[str] = field(default_factory=lambda: ["./player2"])
    creator: str = "p1"  # "p1", "p2", or "shared"
    starting_player: str = "p1"  # "p1" or "p2"
    p1_mark: str = "X"
    p2_mark: str = "O"
    coord_base: int = 0  # 0 or 1
    coord_order: str = "row_col"  # "row_col" or "col_row"
    input_format: Union[str, list[str]] = "{r} {c}\n"
    turn_prompt: Optional[str] = None
    settling_ms: int = 100
    timeout_sec: float = 10.0
    session_timeout_sec: float = 60.0
    env: dict[str, str] = field(default_factory=dict)
    board_extraction: Optional[dict[str, Any]] = None


@dataclass
class Checkpoint:
    """A recorded checkpoint pairing expected state with observed evidence."""

    action_index: int
    target_player: str
    submitted_text: str
    action_summary: str
    expected_board_str: str
    expected_next_player: Optional[str]
    expected_outcome: str
    observed_output: dict[str, str]  # e.g. {"p1": "...", "p2": "..."}
    extracted_board_str: Optional[str] = None
    boundary_reason: str = ""
    timestamp_iso: str = ""


@dataclass
class ProcessInfo:
    """Recorded process state and terminal output."""

    role: str
    pid: Optional[int] = None
    exit_code: Optional[int] = None
    terminated_by_signal: Optional[int] = None
    stdout_transcript: str = ""


@dataclass
class SessionResult:
    """Result of running one scenario for a submission."""

    scenario_id: str
    scenario_name: str
    status: SessionStatus
    status_reason: str = ""
    duration_sec: float = 0.0
    checkpoints: list[Checkpoint] = field(default_factory=list)
    unexecuted_actions: list[Action] = field(default_factory=list)
    transcripts: dict[str, str] = field(default_factory=dict)
    processes: list[ProcessInfo] = field(default_factory=list)
    ipc_state_before: Optional[str] = None
    ipc_state_after: Optional[str] = None
    ipc_leaks_detected: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)


@dataclass
class BuildResult:
    """Result of building a submission."""

    command: Optional[str]
    success: bool
    exit_code: int
    output: str
    duration_sec: float


@dataclass
class SubmissionResult:
    """Aggregated review evidence for a single submission."""

    submission_name: str
    profile_name: str
    profile_data: dict[str, Any]
    timestamp_iso: str
    platform_info: str
    build_result: Optional[BuildResult]
    scenario_results: list[SessionResult] = field(default_factory=list)


@dataclass
class BatchResult:
    """Aggregated results for a batch of submissions."""

    timestamp_iso: str
    suite_name: str
    submissions: list[SubmissionResult] = field(default_factory=list)
