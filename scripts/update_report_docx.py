from pathlib import Path
import shutil
import xml.etree.ElementTree as ET
import zipfile


DOCX_PATH = Path("docs/BaoCao_Suon_FashionStoreSystem.docx")
WORD_NAMESPACE = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
NS = {"w": WORD_NAMESPACE}
ET.register_namespace("w", WORD_NAMESPACE)


PARAGRAPH_UPDATES = {
    7: "Ngày cập nhật sườn: 20/06/2026",
    24: "• Đã có giao diện web demo gồm trang chủ, sản phẩm, giỏ hàng, đăng nhập, thanh toán, staff và admin; giao diện dùng tone đen/trắng chủ đạo trong khi ảnh sản phẩm/editorial giữ màu gốc.",
    25: "• Đã có bộ smoke test kiểm tra core flow, file persistence, order consistency, shipping validation và auth/role guards.",
    27: "• Tiếp tục siết test sâu cho order, payment, shipping, returns và role-based access để khóa các edge case nghiệp vụ.",
    28: "• Dựng HTTP/API adapter thật để web gọi trực tiếp vào core thay vì chỉ dùng local demo state.",
    29: "• Kết nối cart, checkout và payment trên web với phản hồi thật từ backend khi có HTTP layer.",
    30: "• Tối ưu thêm các điểm orchestration hoặc persistence nếu project tiếp tục mở rộng.",
    31: "• Nâng cấp persistence từ file mô phỏng lên SQLite hoặc database thật nếu phạm vi đồ án yêu cầu.",
    36: "Sau giai đoạn hoàn thiện mới nhất, project đã bổ sung thêm các phần còn thiếu quan trọng để báo cáo có đủ nội dung về vận hành hệ thống, thanh toán, giao hàng, API, test và local web integration.",
    39: "• Nhóm 3 - Payment module: bổ sung mô hình Payment base class, payment service và các lớp CashPayment, BankTransferPayment, EWalletPayment để minh họa kế thừa và đa hình trong OOP.",
    40: "• Nhóm 4 - Shipping module: bổ sung shipment, shipping method, quote phí giao hàng, tracking code, trạng thái giao hàng và validation dữ liệu shipment.",
    42: "• Nhóm 6 - Local web shell: bổ sung trang payment, luồng cart → payment → place order, shared demo state và giao diện đen/trắng đồng nhất với ảnh giữ màu gốc.",
    43: "• Nhóm 7 - Docs/test: cập nhật README, architecture overview, báo cáo DOCX và bổ sung smoke test cho order consistency, shipping validation và auth role routing.",
    45: "Nếu tính riêng core OOP C++, project hiện đạt khoảng 90-92%. Nếu tính toàn bộ hệ thống production-like gồm HTTP server thật, database thật và web kết nối backend đầy đủ, project đạt khoảng 78-80%.",
    46: "Các phần còn lại để hoàn chỉnh hơn gồm: dựng HTTP server thật quanh API facade, nối web cart/checkout/payment với response thật từ backend, thay file persistence bằng SQLite hoặc database thật nếu cần, và bổ sung test sâu cho các edge case payment/shipping/admin.",
    48: "Project hiện đã hoàn thành khoảng 90% nếu tính theo trọng tâm core OOP C++ và khoảng 78-80% nếu tính toàn bộ hệ thống gồm backend/API, web kết nối dữ liệu thật và hạ tầng production-like.",
    50: "• Đã có module catalog, inventory, cart, order, pricing/voucher, identity, customer, review, returns, staff, payment, shipping, report, API facade và shared types.",
    52: "• Đã có smoke tests cho core flow, file persistence, order consistency, shipping validation và auth/role guards.",
    53: "• Web hiện đã có các trang landing, product, cart, login, payment, staff và admin dùng chung local demo state.",
    54: "• Các phần còn thiếu lớn: HTTP server thật, database thật, web gọi backend đầy đủ và test sâu hơn cho production edge cases.",
    75: "Mục tiêu là xây dựng core nghiệp vụ bằng C++20, áp dụng OOP, tách domain khỏi persistence và giao diện. Phạm vi hiện tại tập trung vào core, persistence mô phỏng, API facade và local web shell.",
    81: "Tác nhân gồm khách hàng, nhân viên, quản lý/admin. Chức năng gồm đăng nhập, catalog, tồn kho, giỏ hàng, đặt hàng, voucher, thanh toán, giao hàng, review, đổi trả và báo cáo.",
    84: "Kiến trúc gồm domain, application, infrastructure, API facade và web shell. Domain không phụ thuộc layer ngoài; application phụ thuộc repository contract; infrastructure implement contract.",
    87: "Mô tả các module shared, catalog, inventory, cart, order, pricing, identity, customer, review, returns, staff, payment, shipping, report và API facade. Có thể bổ sung UML class diagram ở phần này.",
    90: "Mô tả flow mua hàng: add cart, preview checkout, apply voucher, chuyển sang payment, place order, reserve inventory, mark paid, commit sale, order lifecycle, review và return request.",
    93: "Liệt kê core_smoke, file_persistence_smoke, order_consistency_smoke, shipping_smoke, auth_roles_smoke. Lệnh chạy: powershell -ExecutionPolicy Bypass -File scripts/test-gpp.ps1.",
    96: "Core C++20 đã có domain model, application services, repository pattern, persistence mô phỏng, API facade, local web shell có payment flow và bộ rule coding style trong thư mục rule/.",
    99: "Chưa có HTTP/backend thật, database thật và web chưa gọi live backend end-to-end. Hướng tiếp theo là hoàn thiện các phần này và siết thêm test cho production edge cases.",
    102: "Fashion Store System đã hình thành core OOP C++ tương đối đầy đủ cho nghiệp vụ bán hàng thời trang, đồng thời có local web shell đủ tốt để demo luồng catalog, cart, payment và role-based access.",
}


def set_paragraph_text(paragraph: ET.Element, text: str) -> None:
    text_nodes = paragraph.findall(".//w:t", NS)
    if not text_nodes:
        return
    text_nodes[0].text = text
    for node in text_nodes[1:]:
        node.text = ""


def update_report(docx_path: Path) -> None:
    temp_path = docx_path.with_suffix(docx_path.suffix + ".tmp")

    with zipfile.ZipFile(docx_path, "r") as source_zip:
        root = ET.fromstring(source_zip.read("word/document.xml"))
        paragraphs = root.findall(".//w:body/w:p", NS)

        for index, text in PARAGRAPH_UPDATES.items():
            set_paragraph_text(paragraphs[index], text)

        updated_xml = ET.tostring(root, encoding="utf-8", xml_declaration=True)

        with zipfile.ZipFile(temp_path, "w", zipfile.ZIP_DEFLATED) as target_zip:
            for item in source_zip.infolist():
                data = source_zip.read(item.filename)
                if item.filename == "word/document.xml":
                    data = updated_xml
                target_zip.writestr(item, data)

    shutil.move(temp_path, docx_path)


if __name__ == "__main__":
    update_report(DOCX_PATH)
    print(f"Updated {DOCX_PATH}")
