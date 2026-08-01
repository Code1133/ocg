import argparse
import json
import sys
import tempfile
import zipfile
from contextlib import contextmanager
from pathlib import Path


@contextmanager
def log_group(title: str):
    print(f"::group::{title}", flush=True)
    try:
        yield
    finally:
        print("::endgroup::", flush=True)


def format_uplugin_dict(data: dict, engine_version: str) -> dict:
    """Reorders uplugin dictionary keys so EngineVersion appears near the top."""
    data["EngineVersion"] = engine_version

    preferred_order = [
        "FileVersion",
        "Version",
        "VersionName",
        "EngineVersion",
        "FriendlyName",
        "Description",
        "Category",
        "CreatedBy",
        "CreatedByURL",
        "DocsURL",
        "FabURL",
        "MarketplaceURL",
        "SupportURL",
        "CanContainContent",
        "IsBetaVersion",
        "IsExperimentalVersion",
        "Installed",
        "Modules",
        "Plugins",
    ]

    reordered = {}
    for key in preferred_order:
        if key in data:
            reordered[key] = data[key]

    for key, value in data.items():
        if key not in reordered:
            reordered[key] = value

    return reordered


def dump_uplugin_with_tabs(data: dict) -> str:
    """Dumps dictionary as JSON with Tab indentation instead of Spaces."""
    # indent=4로 1차 직렬화 후 스페이스 4개를 Tab으로 치환
    json_str = json.dumps(data, indent=4, ensure_ascii=False)
    lines = json_str.splitlines()
    tabbed_lines = []

    for line in lines:
        # 줄 앞쪽의 스페이스들을 개수 계산 후 Tab으로 변환
        stripped = line.lstrip(" ")
        spaces = len(line) - len(stripped)
        if spaces > 0 and spaces % 4 == 0:
            tabs = "\t" * (spaces // 4)
            tabbed_lines.append(f"{tabs}{stripped}")
        else:
            tabbed_lines.append(line)

    return "\n".join(tabbed_lines) + "\n"


def package_plugin(
    plugin_dir: Path,
    engine_versions: list[str],
    output_dir: Path,
    custom_name: str | None = None,
    version_override: str | None = None,
) -> None:
    # 1. .uplugin 파일 자동 검색
    uplugin_files = list(plugin_dir.glob("*.uplugin"))
    if not uplugin_files:
        print(
            f"[ERROR] No .uplugin file found in directory: {plugin_dir}",
            file=sys.stderr,
        )
        sys.exit(1)

    uplugin_path = uplugin_files[0]
    plugin_file_name = uplugin_path.name  # 예: OneButtonLevelGeneration.uplugin

    # 플러그인 이름 결정 (사용자 지정 이름이 없으면 .uplugin 파일명에서 추출)
    plugin_name = custom_name or uplugin_path.stem

    with log_group("Parsing Plugin Meta"):
        print(f"[INFO] Found plugin descriptor: {plugin_file_name}")
        print(f"[INFO] Target archive prefix: {plugin_name}")

        # 2. 원본 .uplugin 데이터 읽기 및 버전 파싱
        with uplugin_path.open("r", encoding="utf-8") as f:
            uplugin_data = json.load(f)

        plugin_version = version_override or uplugin_data.get("VersionName", "1.0.0")
        print(f"[INFO] Target Plugin Version: {plugin_version}")

    output_dir.mkdir(parents=True, exist_ok=True)

    # 에픽 TRC 검수 기준 불필요 폴더/확장자 제외
    exclude_dirs = {
        ".devcontainer",
        ".git",
        ".github",
        ".vscode",
        "Binaries",
        "Build",
        "Docs",
        "Intermediate",
        "Saved",
        output_dir.resolve().name,
    }
    exclude_exts = {
        ".db",
        ".DS_Store",
        ".gitignore",
        ".opendb",
        ".py",
        ".sln",
        ".user",
    }

    uplugin_data["VersionName"] = plugin_version

    # 3. 엔진 버전별로 패키징
    for ver in engine_versions:
        ver = ver.strip()
        major_minor = ".".join(ver.split(".")[:2])
        full_engine_ver = f"{major_minor}.0"
        zip_name = f"{plugin_name}_v{plugin_version}_UE{major_minor}.zip"
        zip_path = output_dir / zip_name

        with log_group(f"Packaging for UE {major_minor}"):
            print(f"[PROCESS] EngineVersion set to: {full_engine_ver}")

            # Tab 들여쓰기가 적용된 JSON 문자열 생성
            formatted_data = format_uplugin_dict(uplugin_data, full_engine_ver)
            formatted_json_str = dump_uplugin_with_tabs(formatted_data)

            with tempfile.TemporaryDirectory() as temp_dir:
                temp_uplugin_path = Path(temp_dir) / plugin_file_name
                with temp_uplugin_path.open("w", encoding="utf-8") as f:
                    f.write(formatted_json_str)

                # ZIP 패키징 (최상위에 바로 .uplugin 및 소스가 오도록 압축)
                file_count = 0
                with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
                    # 재귀적으로 plugin_dir를 탐색하며 파일을 압축
                    for path in plugin_dir.rglob("*"):
                        # 만약 경로의 일부가 제외 디렉토리 목록에 포함되어 있으면 건너뜀
                        if any(part in exclude_dirs for part in path.parts):
                            continue

                        # 확장자 필터링
                        if path.is_file() and path.suffix not in exclude_exts:
                            rel_path = Path(plugin_name.lower()) / path.relative_to(plugin_dir)

                            if path == uplugin_path:
                                z.write(temp_uplugin_path, rel_path)
                            else:
                                z.write(path, rel_path)

                            file_count += 1

            print(f"[SUCCESS] Archive created: {zip_path} ({file_count} files)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Unreal Engine Plugin Packaging Tool")
    parser.add_argument(
        "--dir",
        type=Path,
        default=Path("./"),
        help="Path to plugin root directory (Default: ./)",
    )
    parser.add_argument(
        "--engines",
        required=True,
        help="Comma-separated engine versions (e.g. '5.5,5.6,5.7,5.8')",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("./dist"),
        help="Output directory path (Default: ./dist)",
    )
    parser.add_argument(
        "--name",
        default="OCG",
        help="Custom plugin prefix name for zip files (Default: OCG)",
    )
    parser.add_argument("--version", default=None, help="Manual version override")

    args = parser.parse_args()
    engine_list = args.engines.split(",")

    package_plugin(
        plugin_dir=args.dir,
        engine_versions=engine_list,
        output_dir=args.output,
        custom_name=args.name,
        version_override=args.version,
    )
