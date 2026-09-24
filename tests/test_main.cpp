#include "tree_crdt/document.hpp"
#include <iostream>
#include <cassert>

using namespace tree_crdt;

void test_basic_tree() {
    std::cout << "[Test] Running Basic Tree & Insertion Test..." << std::endl;
    Document doc(1);

    doc.insert(0, "world");
    doc.insert(0, "Hello ");
    doc.insert(doc.length(), "!");

    assert(doc.to_string() == "Hello world!");
    assert(doc.length() == 12);
    std::cout << "  -> Document content: \"" << doc.to_string() << "\" (Length: " << doc.length() << ")" << std::endl;
    std::cout << "  -> Basic Tree Test: PASSED\n" << std::endl;
}

void test_deletion_and_tombstones() {
    std::cout << "[Test] Running Deletion & Tombstones Test..." << std::endl;
    Document doc(1);

    doc.insert(0, "Hello cruel world!");
    assert(doc.to_string() == "Hello cruel world!");

    // Delete "cruel " (6 characters starting at index 6)
    doc.delete_range(6, 6);
    assert(doc.to_string() == "Hello world!");
    assert(doc.length() == 12);

    std::cout << "  -> Content after deletion: \"" << doc.to_string() << "\"" << std::endl;
    std::cout << "  -> Deletion & Tombstone Test: PASSED\n" << std::endl;
}

void test_monaco_2d_coordinates() {
    std::cout << "[Test] Running Monaco 2D Coordinate Translation Test..." << std::endl;
    Document doc(1);

    doc.insert(0, "function add(a, b) {\n    return a + b;\n}");

    // Index 0 -> Ln 1, Col 1
    EditorPosition p0 = doc.index_to_position(0);
    assert(p0.line == 1 && p0.column == 1);

    // After first newline ("function add(a, b) {\n") -> Length is 21
    EditorPosition p_line2 = doc.index_to_position(21);
    assert(p_line2.line == 2 && p_line2.column == 1);

    std::cout << "  -> Index 0 translates to: " << p0 << std::endl;
    std::cout << "  -> Index 21 translates to: " << p_line2 << std::endl;
    std::cout << "  -> Monaco 2D Coordinates Test: PASSED\n" << std::endl;
}

void test_decentralized_yata_convergence() {
    std::cout << "[Test] Running Decentralized YATA Convergence Test..." << std::endl;

    // Simulate two distributed peers
    Document docA(101);
    Document docB(202);

    // Initial synchronized state
    docA.insert(0, "Hello world");
    auto initial_update = docA.encode_update(docB.get_state_vector());
    docB.apply_update(initial_update);

    assert(docA.to_string() == "Hello world");
    assert(docB.to_string() == "Hello world");

    // Network partition: Both peers edit concurrently at the same spot (after "Hello ")
    // DocA inserts "brave "
    docA.insert(6, "brave ");
    // DocB inserts "collaborative "
    docB.insert(6, "collaborative ");

    std::cout << "  -> DocA offline state: \"" << docA.to_string() << "\"" << std::endl;
    std::cout << "  -> DocB offline state: \"" << docB.to_string() << "\"" << std::endl;

    // Peers reconnect and exchange binary delta updates
    auto updateA_for_B = docA.encode_update(docB.get_state_vector());
    auto updateB_for_A = docB.encode_update(docA.get_state_vector());

    docB.apply_update(updateA_for_B);
    docA.apply_update(updateB_for_A);

    // Verify 100% mathematical convergence
    std::cout << "  -> DocA merged state: \"" << docA.to_string() << "\"" << std::endl;
    std::cout << "  -> DocB merged state: \"" << docB.to_string() << "\"" << std::endl;

    assert(docA.to_string() == docB.to_string());
    std::cout << "  -> Decentralized YATA Convergence Test: PASSED (Strong Eventual Consistency Achieved!)\n" << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   TreeCRDT (C++20 Engine) Automated Test Suite   " << std::endl;
    std::cout << "==================================================\n" << std::endl;

    test_basic_tree();
    test_deletion_and_tombstones();
    test_monaco_2d_coordinates();
    test_decentralized_yata_convergence();

    std::cout << "==================================================" << std::endl;
    std::cout << "   ALL TREE-CRDT C++20 TESTS PASSED SUCCESSFULLY! " << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
