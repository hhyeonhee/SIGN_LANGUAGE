import os
import json
import time
import logging
import pymysql
import http.server
import socket
import socketserver
import threading
from moviepy.editor import VideoFileClip, concatenate_videoclips
from konlpy.tag import Komoran
from sklearn.metrics.pairwise import cosine_similarity # 유사도
import numpy as np
from transformers import AutoTokenizer, AutoModel
from datetime import datetime
import torch

VIDEO_BASE_PATH = r"F:\Signvideos"  # 실제 경로
OUTPUT_DIR = "composed"
HTTP_BASE_URL = "http://10.10.20.117:8000"

logging.basicConfig(level=logging.INFO)

def get_db():
    return pymysql.connect(
        host="10.10.20.107",
        user="jinyoung",
        password="1234",
        database="SUHWA",
        charset="utf8mb4",
        cursorclass=pymysql.cursors.Cursor
    )

categories = [
            'category_animals', 'category_cloth', 'category_concept', 
            'category_culture', 'category_daily', 'category_economy',
            'category_edu', 'category_food', 
            'category_human', 'category_life', 'category_loca',
            'category_nature', 'category_politics', 'category_religion', 
            'category_social'
]

detailed_categories = [
    'animals', 'insects', 'plants', 'part_of_an_animal', 'part_of_a_plant',
    'animal_and_plant_activities', 'types_of_clothes', 'part_of_clothes',
    'hats_shoes_jewelry', 'wearing_of_clothing', 'shape', 'property', 'speed',
    'brightness', 'color', 'number', 'quantity', 'degree', 'sequence', 'frequency',
    'time', 'location_and_direction', 'area', 'instruction', 'conjunction',
    'question', 'personal_name', 'cultural_agents', 'music', 'art', 'literature', 'art_2',
    'cultural_activities', 'traditional_culture', 'place_of_cultural_life',
    'type_of_building', 'residential_areas_and_forms', 'daily_supplies',
    'composition_of_house', 'state_of_residence', 'housekeeping', 'economic_actors',
    'place_of_economic_activity', 'economic_instruments', 'state_of_economy',
    'economic_activity', 'subject_of_teaching_and_learning', 'curriculum_and_subjects',
    'educational_institution', 'school_facilities', 'learning-related_objects',
    'academic_terminology', 'teaching_and_learning_behavior', 'food', 'vegetable',
    'grain_grains', 'fruit', 'beverage', 'ingredient', 'meals_and_cooking',
    'types_of_people', 'body_parts_interior', 'physiological_phenomenon', 'sense',
    'emotion', 'personality_attitude', 'appearance', 'ability', 'physical_behavior',
    'cognitive_behavior', 'state_of_life', 'act_of_life', 'family_relationships',
    'leisure_life', 'illnesses_and_symptoms', 'treatment', 'sports_and_games',
    'name_of_country', 'place_name', 'wear_and_temperature', 'lay_of__land',
    'surface_object', 'celestial_bodies', 'resource', 'disaster', 'public_institution',
    'judicial_and_security_agents', 'judicial_and_security_practices',
    'state_of_politics_and_security', 'political_and_administrative_entities',
    'political_and_administrative_actions', 'weapon', 'type_of_religion',
    'object_of_faith', 'religious_man', 'place_of_religious_life',
    'religious_language', 'human_relationship', 'communication', 'Media',
    'working_life', 'occupation', 'social_event', 'social_activities', 'language', 'traffic'
]

komoran = Komoran()
tokenizer = AutoTokenizer.from_pretrained("beomi/kcbert-base")
model = AutoModel.from_pretrained("beomi/kcbert-base")

def get_sentence_embedding(text):
    try:
        inputs = tokenizer(
            text,
            return_tensors="pt",
            padding=True,
            truncation=True,
            max_length=300
        )
        if inputs['input_ids'].shape[1] == 0:
            raise ValueError("토큰이 없음")

        with torch.no_grad():
            outputs = model(**inputs)
            if outputs.last_hidden_state.shape[1] == 0:
                raise ValueError("임베딩 결과 없음")

            embedding = outputs.last_hidden_state.mean(dim=1).squeeze()

        return embedding.numpy()

    except Exception as e:
        logging.warning(f"[get_sentence_embedding 예외 발생] '{text[:30]}...' → {e}")
        raise  # 예외는 다시 상위로 전달 (for loop에서 logging 찍음)

def compute_subcategory_embeddings():
    db = get_db()
    cur = db.cursor()
    new_embeddings = {}
    global subcategory_to_table
    subcategory_to_table = {}

    for table in categories:
        try:
            cur.execute(f"SELECT DISTINCT sub_category_en_name FROM {table} ORDER BY sub_category_num")
            sub_categories = [row[0] for row in cur.fetchall() if row[0]]

            for sub_cat in sub_categories:
                if sub_cat not in detailed_categories:
                    logging.warning(f"[불일치] {sub_cat} → detailed_categories에 없음")
                    continue

                cur.execute(f"""
                    SELECT kr_name, sub_category_kr_name, sub_category_en_name 
                    FROM {table} 
                    WHERE sub_category_en_name = %s 
                    ORDER BY sub_category_num
                """, (sub_cat,))
                rows = cur.fetchall()

                full_texts = []
                for row in rows:
                    parts = [w.strip() for w in row if w and w.strip()]
                    full_texts.append(" ".join(parts))

                full_texts = list(dict.fromkeys(full_texts))[:1500]
                if not full_texts:
                    logging.warning(f"[❗️텍스트 없음] {sub_cat} in {table}")
                    continue

                try:
                    emb = get_sentence_embedding(" ".join(full_texts))
                    new_embeddings[sub_cat] = emb.tolist()
                    if sub_cat not in subcategory_to_table:
                        subcategory_to_table[sub_cat] = table
                        logging.info(f"[🔗 등록] {sub_cat} → {table}")
                    else:
                        logging.info(f"[⛔ 중복] {sub_cat} → 이미 {subcategory_to_table[sub_cat]}에 등록됨")

                    logging.info(f"[✅ 임베딩 완료] {sub_cat} ← {table}")
                except Exception as e:
                    logging.warning(f"[임베딩 실패] {sub_cat} in {table} → {e}")

        except Exception as e:
            logging.error(f"[DB 조회 실패] {table} → {e}")
            continue

    db.close()
    return new_embeddings




embedding_map = {}

def extract_meaningful_tokens(text):
    tokens = komoran.pos(text)
    keywords = []
    i = 0
    while i < len(tokens):
        word, pos = tokens[i]

        if pos in ('VV', 'VA'):
            lemma = word
            while i + 1 < len(tokens) and tokens[i + 1][1] in ('EP', 'EC', 'EF'):
                lemma += tokens[i + 1][0]
                i += 1
            keywords.append(lemma)

        elif pos in ('NNG', 'NNP', 'XR'):
            keywords.append(word)

        i += 1

    return keywords



# 형태소 분석 후 어간 원형으로 복원 
def extract_meaningful_tokens_and_store(text, sentence_id=None):
    tokens = komoran.pos(text)
    db = get_db()
    cur = db.cursor()

    keywords = []
    position = 0
    i = 0

    while i < len(tokens):
        word, pos = tokens[i]
        is_keyword = 0

        if pos in ('VV', 'VA'):
            lemma = word
            while i + 1 < len(tokens) and tokens[i + 1][1] in ('EP', 'EC', 'EF'):
                lemma += tokens[i + 1][0]
                i += 1
            is_keyword = 1
            keywords.append(lemma)

            if sentence_id is not None:
                cur.execute("""
                    INSERT INTO token (sentence_id, token_text, pos_tag, position, is_keyword)
                    VALUES (%s, %s, %s, %s, %s)
                """, (sentence_id, lemma, pos, position, is_keyword))
            position += 1

        elif pos in ('NNG', 'NNP', 'XR'):
            is_keyword = 1
            keywords.append(word)

            if sentence_id is not None:
                cur.execute("""
                    INSERT INTO token (sentence_id, token_text, pos_tag, position, is_keyword)
                    VALUES (%s, %s, %s, %s, %s)
                """, (sentence_id, word, pos, position, is_keyword))
            position += 1

        i += 1

    if sentence_id is not None:
        db.commit()

    db.close()
    return keywords



# def update_embedding_map_from_text(text):
#     tokens = extract_meaningful_tokens(text)  # ✅ 안전하게 DB insert 없이!
#     embedding_map.clear()
    
#     for sub_cat, vec in table_embeddings.items():
#         for word in tokens:
#             key = f"{sub_cat}:{word}"
#             # perturb = np.random.normal(0, 0.05, size=len(vec)) # noise 제거 <25일 수정>
#             # embedding_map[key] = (np.array(vec) + perturb).tolist()
#             embedding_map[key] = vec
            
#     return tokens

def update_embedding_map_from_text(text):
    tokens = extract_meaningful_tokens_and_store(text, sentence_id=None) # 어간 복원한 토큰
    embedding_map.clear()

    db = get_db()
    cur = db.cursor()

    for sub_cat, vec in table_embeddings.items():
        for word in tokens:
            # DB에 실제 단어 존재할 때만 embedding_map 등록
            try:
                cur.execute(f"SELECT COUNT(*) FROM {subcategory_to_table[sub_cat]} WHERE kr_name = %s", (word,))
                count = cur.fetchone()[0]
                if count > 0:
                    key = f"{sub_cat}:{word}"
                    if key not in embedding_map:
                        embedding_map[key] = vec
                        logging.debug(f"[등록] {key} ← {subcategory_to_table[sub_cat]}")
            except Exception as e:
                logging.warning(f"[조회 실패] {sub_cat}:{word} → {e}")
                continue

    db.close()
    return tokens




def get_all_tables():
    return list(table_embeddings.keys())

def table_to_dir(table):
    """
    DB 테이블명(category_concept 등)을 실제 로컬 폴더명(Category_Concept)으로 변환
    """
    return "Category_" + table.replace("category_", "").capitalize()


def select_all_video_paths(token):
    db = get_db()
    cur = db.cursor()
    result = {}

    for sub_cat in table_embeddings:
        table = subcategory_to_table.get(sub_cat)
        if not table:
            logging.warning(f"[❌ subcategory_to_table 누락] {sub_cat}")
            continue

        dir_name = table_to_dir(table)

        # 1. 정확 일치 검색
        cur.execute(f"SELECT kr_name, video_path FROM {table} WHERE kr_name = %s", (token,))
        exact_rows = cur.fetchall()
        logging.info(f"[{table}] 정확 일치 개수: {len(exact_rows)}")

        if exact_rows:
            try:
                best_row = max(
                    exact_rows,
                    key=lambda row: cosine_similarity(
                        np.array(get_sentence_embedding(row[0])).reshape(1, -1),
                        np.array([table_embeddings[sub_cat]]).reshape(1, -1)
                    )[0][0]
                )
                video_path = (
                    best_row[1]
                    .replace("\\", "/")
                    .replace("video/", "")
                    .replace("./", "")
                )
                if not video_path.lower().endswith(".mp4"):
                    video_path += ".mp4"

                abs_path = os.path.normpath(os.path.join(VIDEO_BASE_PATH, video_path))


                if os.path.exists(abs_path):
                    result[sub_cat] = abs_path
                    logging.info(f"[정확 일치] {token} → {abs_path}")
                else:
                    logging.warning(f"[⚠️ 파일 없음] {abs_path}")
                continue
            except Exception as e:
                logging.warning(f"[정확 일치 유사도 오류] {token} in {sub_cat} → {e}")
                continue

        # 2. LIKE 검색
        cur.execute(f"SELECT kr_name, video_path FROM {table} WHERE kr_name LIKE %s", (f"%{token}%",))
        rows = cur.fetchall()
        logging.info(f"[{table}] LIKE 결과 개수: {len(rows)}")

        if not rows:
            continue

        try:
            best_row = max(
                rows,
                key=lambda row: cosine_similarity(
                    np.array(get_sentence_embedding(row[0])).reshape(1, -1),
                    np.array([table_embeddings[sub_cat]]).reshape(1, -1)
                )[0][0]
            )
            video_path = (
                best_row[1]
                .replace("\\", "/")
                .replace("video/", "")
                .replace("./", "")
            )
            if not video_path.lower().endswith(".mp4"):
                video_path += ".mp4"

            abs_path = os.path.normpath(os.path.join(VIDEO_BASE_PATH, video_path))


            if os.path.exists(abs_path):
                result[sub_cat] = abs_path
                logging.info(f"[후보 발견] {token} → {abs_path}")
            else:
                logging.warning(f"[⚠️ 파일 없음] {abs_path}")
        except Exception as e:
            logging.warning(f"[LIKE 유사도 오류] {token} in {sub_cat} → {e}")
            continue

    db.close()
    return result



    # for sub_cat in table_embeddings:
    #     table = subcategory_to_table.get(sub_cat)
    #     logging.info(f"[tables] : {table}")
    #     logging.info(f"[TOKEN DEBUG] '{token}' (len={len(token)})")

    #     if not table:
    #         logging.warning(f"[❌ 테이블 없음] sub_cat: {sub_cat}")
    #         continue

    #     dir_name = table_to_dir(table)
    #     cur.execute(f"SELECT kr_name, video_path FROM {table} WHERE kr_name = %s", (token,))
    #     exact_rows = cur.fetchall()

    #     if exact_rows:
    #         try:
    #             best_row = max(exact_rows, key=lambda row: cosine_similarity(
    #                 np.array(get_sentence_embedding(row[0])).reshape(1, -1),
    #                 np.array([table_embeddings[sub_cat]]).reshape(1, -1)
    #             )[0][0])
    #             abs_path = os.path.join(VIDEO_BASE_PATH, dir_name, best_row[1])
    #             if os.path.exists(abs_path):
    #                 result[sub_cat] = abs_path
    #                 logging.info(f"[정확 일치] {token} → {abs_path}")
    #             continue
    #         except Exception as e:
    #             logging.warning(f"[정확 일치 유사도 오류] {token} in {sub_cat} → {e}")
    #             continue

    if not result:
        for sub_cat in table_embeddings:
            table = subcategory_to_table.get(sub_cat)
            if not table:
                logging.warning(f"[❌ 테이블 없음 - LIKE] sub_cat: {sub_cat}")
                continue

            dir_name = table_to_dir(table)
            cur.execute(f"SELECT kr_name, video_path FROM {table} WHERE kr_name LIKE %s", (f"%{token}%",))
            rows = cur.fetchall()
            if not rows:
                continue

            try:
                best_row = max(rows, key=lambda row: cosine_similarity(
                    np.array(get_sentence_embedding(row[0])).reshape(1, -1),
                    np.array([table_embeddings[sub_cat]]).reshape(1, -1)
                )[0][0])
                abs_path = os.path.join(VIDEO_BASE_PATH, dir_name, best_row[1])
                if os.path.exists(abs_path):
                    result[sub_cat] = abs_path
                    logging.info(f"[후보 발견] {token} → {abs_path}")
            except Exception as e:
                logging.warning(f"[LIKE 유사도 오류] {token} in {sub_cat} → {e}")
                continue

    db.close()
    return result

def select_best_category_from_candidates(token, context_emb, candidates):
    # ✅ 정확 일치 로직 수정 - candidates는 {table: video_path} 형태
    exact_matches = []
    for table in candidates:
        key = f"{table}:{token}"
        if key in embedding_map:
            exact_matches.append(table)
    
    if exact_matches:
        best_table = max(exact_matches, key=lambda t: cosine_similarity(context_emb, np.array([embedding_map[f"{t}:{token}"]]))[0][0])
        logging.info(f"[정확 일치 우선] {token} → {best_table}")        
        return best_table

    # 유사도 기반 선택 로직
    best_score = -1
    best_table = None
    for table in candidates:
        key = f"{table}:{token}"
        if key in embedding_map:
            emb = np.array(embedding_map[key]).reshape(1, -1)
            # score = cosine_similarity(context_emb, emb)[0][0]
            score = cosine_similarity(context_emb, np.array(embedding_map[f"{table}:{token}"]).reshape(1, -1))[0][0] # 25일 수정
            logging.info(f"[유사도] {token} - {table} → {score:.4f}")
                       
            if score > best_score:
                best_score = score
                best_table = table
    return best_table


def compose_video(video_paths, output_dir="composed"):
    clips = [VideoFileClip(path) for path in video_paths if os.path.exists(path)]
    if not clips:
        raise ValueError("비디오 파일 없음")

    final = concatenate_videoclips(clips)

    # 날짜 기반 파일명 + 넘버링
    today_str = time.strftime("%Y%m%d")  # 예: 20250724
    os.makedirs(output_dir, exist_ok=True)

    existing = [
        f for f in os.listdir(output_dir)
        if f.startswith(f"composed_{today_str}_") and f.endswith(".mp4")
    ]
    numbers = [
        int(f.split("_")[-1].split(".")[0])
        for f in existing if f.split("_")[-1].split(".")[0].isdigit()
    ]
    next_number = max(numbers, default=0) + 1
    filename = f"composed_{today_str}_{next_number:03d}.mp4"
    output_path = os.path.join(output_dir, filename)

    final.write_videofile(output_path, logger=None)
    return output_path

def generate_url(path):
    return f"http://10.10.20.117:8000/{path.replace(os.sep, '/')}"

def send_result_to_server(client_fd, url):
    data = {
        "PROTOCOL": "10",
        "CLIENT_FD": str(client_fd),
        "VIDEO_PATH": url
    }
    sock = None  # ✅ 변수 초기화
    try:
        sock = socket.create_connection(("10.10.20.108", 12345))
        sock.sendall(json.dumps(data).encode())
        logging.info(f"결과 전송 완료: {data}")
    except Exception as e:
        logging.error(f"전송 실패: {e}")
    finally:
        if sock:
            sock.close()

def process(text, client_fd, client_ip="unknown"):
    db = get_db()
    cur = db.cursor()

    try:         
        # 1. 문장을 sentence 테이블에 저장하고 ID 확보
        cur.execute("INSERT INTO sentences (raw_text) VALUES (%s)", (text,))
        sentence_id = cur.lastrowid

        # ✅ 1-2. 검색 기록 저장
        history_id = _insert_search_history(cur, text, client_ip)
        logging.info(f"[기록 저장] sentence_id={sentence_id}, history_id={history_id}")
        
        db.commit()
    except Exception as e:
        logging.error(f"[DB 저장 실패] 문장 → {e}")
        return
    finally:
        db.close()

    try:
        # 2. 의미 있는 토큰 추출 및 token 테이블 저장
        keywords = extract_meaningful_tokens_and_store(text, sentence_id)
    except Exception as e:
        logging.error(f"[형태소 분석/저장 실패] → {e}")
        return

    try:
        # 3. 문장 임베딩 추출
        context_emb = get_sentence_embedding(text).reshape(1, -1)
    except Exception as e:
        logging.error(f"[문장 임베딩 실패] → {e}")
        return
    tokens = update_embedding_map_from_text(text)  # ⬅️ 이 줄을 process()에서 3번 이후에 반드시 호출!

    # 4. 각 토큰에 대해 가장 유사한 비디오 선택
    selected_paths = []
    for token in keywords:
        try:
            logging.info(f"[token] {token}")
            candidates = select_all_video_paths(token)
            if not candidates:
                logging.warning(f"[{token}] 후보 없음")
                continue

            best_table = select_best_category_from_candidates(token, context_emb, candidates)
            best_path = candidates.get(best_table)
            if best_path:
                selected_paths.append(best_path)
                logging.info(f"[선택 결과] {token} → {best_table}:{best_path}")
            else:
                logging.warning(f"[{token}] best_path 없음")

        except Exception as e:
            logging.error(f"[{token}] 처리 중 오류 → {e}")

    # 5. 비디오 조합 및 전송
    if selected_paths:
        try:
            final_video = compose_video(selected_paths)
            url = generate_url(final_video)
            send_result_to_server(client_fd, url)
        except Exception as e:
            logging.error(f"[비디오 조합 실패] {e}")
    else:
        logging.warning("전송할 비디오 없음.")

def _insert_sentence(cursor, raw_text):
    """SENTENCES 테이블에 문장을 저장합니다."""
    sql_insert_sentence = "INSERT INTO sentences (raw_text, created_at) VALUES (%s, NOW())"
    cursor.execute(sql_insert_sentence, (raw_text,))
    return cursor.lastrowid

def _insert_search_history(cursor, search_content, client_ip=None):
    """search_history 테이블에 검색 기록을 저장합니다."""
    sql_insert_history = """
        INSERT INTO search_history (history_date, history_content, history_ip)
        VALUES (NOW(), %s, %s)
    """
    cursor.execute(sql_insert_history, (search_content, client_ip))
    return cursor.lastrowid

def handle_client(sock, addr):
    with sock:
        while True:
            try:
                data = sock.recv(4096)
                if not data:
                    break
                msg = json.loads(data.decode())
                text = msg.get("TEXT", "")
                client_fd = msg.get("CLIENT_FD", "")
                client_ip = addr[0]
                logging.info(f"[수신] TEXT: {text}, CLIENT_FD: {client_fd}")
                process(text, client_fd, client_ip)
            except Exception as e:
                logging.error(f"클라이언트 처리 실패: {e}")
                break

def start_server(host="0.0.0.0", port=12345):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(5)
    logging.info(f"파이썬 서버 실행 중... {host}:{port}")
    while True:
        client_sock, addr = server.accept()
        threading.Thread(target=handle_client, args=(client_sock, addr), daemon=True).start()


# 비디오 HTTP서버 
def start_http_server(directory="composed", port=8000):
    
    os.makedirs(directory, exist_ok=True)
    handler = http.server.SimpleHTTPRequestHandler
    os.chdir(directory)

    def serve():
        with socketserver.TCPServer(("0.0.0.0", port), handler) as httpd:
            logging.info(f"[HTTP 서버 실행] http://0.0.0.0:{port}/ → 영상 제공")
            httpd.serve_forever()

    threading.Thread(target=serve, daemon=True).start()

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)  # ✅ 여기에 명확히 재정의해보기
    logging.info("[MAIN] 임베딩 시작")
    table_embeddings = compute_subcategory_embeddings()
    logging.info("[MAIN] 임베딩 완료")

    start_http_server("composed", 8000)
    start_server()
